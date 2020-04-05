/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/kgr/net/MasterServer.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <set>
#include <thread>

#include "sloked/core/Error.h"
#include "sloked/kgr/local/Pipe.h"
#include "sloked/kgr/net/Config.h"
#include "sloked/kgr/net/Interface.h"
#include "sloked/sched/ScopedExecutor.h"

using namespace std::chrono_literals;

namespace sloked {

    class KgrMasterNetServerContext : public SlokedIOPoller::Awaitable {
     public:
        KgrMasterNetServerContext(std::unique_ptr<SlokedSocket> socket,
                                  const std::atomic<bool> &work,
                                  SlokedCounter<std::size_t>::Handle counter,
                                  SlokedExecutor &threadManager,
                                  KgrNamedServer &server,
                                  SlokedNamedRestrictionAuthority *restrictions,
                                  SlokedAuthenticatorFactory *authFactory)
            : net(std::move(socket)), work(work), server(server), nextPipeId(0),
              counterHandle(std::move(counter)), workers{threadManager},
              pinged{false}, restrictions(restrictions) {

            if (authFactory) {
                this->auth = authFactory->NewMaster(this->net.GetEncryption());
            }

            this->lastActivity = std::chrono::system_clock::now();

            this->net.BindMethod(
                "connect", [this](const std::string &method,
                                  const KgrValue &params, auto &rsp) {
                    const auto &service = params.AsString();
                    std::unique_lock lock(this->mtx);
                    if (!this->IsAccessPermitted(service)) {
                        rsp.Error("KgrMasterServer: Access to \'" + service +
                                  "\' restricted");
                    } else {
                        this->workers.Enqueue(
                            [this, service, rsp = std::move(rsp)]() mutable {
                                auto pipe = this->server.Connect({service});
                                if (pipe == nullptr) {
                                    throw SlokedError(
                                        "KgrMasterServer: Pipe can't be null");
                                } else {
                                    this->Connect(std::move(pipe), rsp);
                                }
                            });
                    }
                });

            this->net.BindMethod(
                "activate", [this](const std::string &method,
                                   const KgrValue &params, auto &rsp) {
                    std::unique_lock lock(this->mtx);
                    const auto pipeId = params.AsInt();
                    if (this->frozenPipes.count(pipeId) != 0) {
                        this->frozenPipes.erase(pipeId);
                        auto &pipe = *this->pipes.at(pipeId);
                        while (!pipe.Empty()) {
                            this->net.Invoke(
                                "send", KgrDictionary{{"pipe", pipeId},
                                                      {"data", pipe.Read()}});
                        }
                        if (pipe.GetStatus() == KgrPipe::Status::Closed) {
                            this->net.Invoke("close", pipeId);
                            this->pipes.erase(pipeId);
                        }
                    }
                });

            this->net.BindMethod(
                "send", [this](const std::string &method,
                               const KgrValue &params, auto &rsp) {
                    std::unique_lock lock(this->mtx);
                    const auto pipeId = params.AsDictionary()["pipe"].AsInt();
                    const auto &data = params.AsDictionary()["data"];
                    if (this->pipes.count(pipeId)) {
                        auto &pipe = *this->pipes.at(pipeId);
                        pipe.Write(KgrValue{data});
                        rsp.Result(true);
                    } else {
                        rsp.Result(false);
                    }
                });

            this->net.BindMethod("close",
                                 [this](const std::string &method,
                                        const KgrValue &params, auto &rsp) {
                                     std::unique_lock lock(this->mtx);
                                     const auto pipeId = params.AsInt();
                                     if (this->pipes.count(pipeId)) {
                                         auto &pipe = *this->pipes.at(pipeId);
                                         pipe.Close();
                                         this->pipes.erase(pipeId);
                                     }
                                 });

            this->net.BindMethod(
                "bind", [this](const std::string &method,
                               const KgrValue &params, auto &rsp) {
                    const auto &service = params.AsString();
                    if (!this->IsModificationPermitted(service)) {
                        rsp.Error("KgrMasterServer: Modification of \'" +
                                  service + "\' restricted");
                    } else {
                        this->workers.Enqueue([this, service,
                                               rsp = std::move(rsp)]() mutable {
                            auto res = this->server.Registered({service});
                            res.Wait();
                            if (res.State() == TaskResultStatus::Ready &&
                                !res.Unwrap()) {
                                this->server
                                    .Register({service},
                                              std::make_unique<SlaveService>(
                                                  *this, service))
                                    .Wait();
                                std::unique_lock lock(this->mtx);
                                this->remoteServiceList.insert(service);
                                rsp.Result(true);
                            } else {
                                rsp.Result(false);
                            }
                        });
                    }
                });

            this->net.BindMethod(
                "bound", [this](const std::string &method,
                                const KgrValue &params, auto &rsp) {
                    this->workers.Enqueue([this, params,
                                           rsp = std::move(rsp)]() mutable {
                        const auto &service = params.AsString();
                        auto res = this->server.Registered({service});
                        res.Wait();
                        rsp.Result(res.Unwrap() &&
                                   (this->IsAccessPermitted(service) ||
                                    this->IsModificationPermitted(service)));
                    });
                });

            this->net.BindMethod(
                "unbind", [this](const std::string &method,
                                 const KgrValue &params, auto &rsp) {
                    const auto &service = params.AsString();
                    if (!this->IsModificationPermitted(service)) {
                        rsp.Error("KgrMasterServer: Modification of \'" +
                                  service + "\' restricted");
                    } else {
                        this->workers.Enqueue([this, service,
                                               rsp = std::move(rsp)]() mutable {
                            if (this->remoteServiceList.count(service) != 0) {
                                this->server.Deregister({service});
                                std::unique_lock lock(this->mtx);
                                this->remoteServiceList.erase(service);
                                rsp.Result(true);
                            } else {
                                rsp.Result(false);
                            }
                        });
                    }
                });

            this->net.BindMethod(
                "ping", [](const std::string &method, const KgrValue &params,
                           auto &rsp) { rsp.Result("pong"); });

            this->net.BindMethod("auth-request", [this](
                                                     const std::string &method,
                                                     const KgrValue &params,
                                                     auto &rsp) {
                if (this->auth) {
                    auto nonce = this->auth->InitiateLogin();
                    rsp.Result(
                        KgrDictionary{{"nonce", static_cast<int64_t>(nonce)}});
                } else {
                    rsp.Error("KgrMasterServer: Authentication not supported");
                }
            });

            this->net.BindMethod("auth-response", [this](
                                                      const std::string &method,
                                                      const KgrValue &params,
                                                      auto &rsp) {
                if (this->auth) {
                    const auto &keyId = params.AsDictionary()["id"].AsString();
                    const auto &result =
                        params.AsDictionary()["result"].AsString();
                    auto res = this->auth->ContinueLogin(keyId, result);
                    rsp.Result(res);
                    if (res) {
                        this->auth->FinalizeLogin();
                    }
                } else {
                    rsp.Error("KgrMasterServer: Authentication not supported");
                }
            });
        }

        virtual ~KgrMasterNetServerContext() {
            this->workers.Close();
            std::unique_lock lock(this->mtx);
            for (const auto &pipe : this->pipes) {
                pipe.second->Close();
            }
            this->pipes.clear();
            if (this->net.Valid()) {
                this->net.Close();
            }
            auto &srv = this->server;
            std::thread([handle = this->counterHandle,
                         services = std::move(this->remoteServiceList), &srv] {
                for (const auto &rService : services) {
                    srv.Deregister({rService});
                }
            }).detach();
        }

        std::unique_ptr<SlokedIOAwaitable> GetAwaitable() const final {
            return this->net.Awaitable();
        }

        void Process(bool success) final {
            if (success || this->net.Available() > 0) {
                this->pinged = false;
                this->Accept();
            } else if (this->work.load() && this->net.Valid()) {
                auto now = std::chrono::system_clock::now();
                auto idle = now - this->lastActivity;
                if (idle > KgrNetConfig::InactivityThreshold && this->pinged) {
                    throw SlokedError(
                        "KgrMasterServer: Connection inactive for " +
                        std::to_string(idle.count()) + " ns");
                } else if (idle > KgrNetConfig::InactivityTimeout &&
                           !this->pinged) {
                    this->net.Invoke("ping", {});
                    this->pinged = true;
                }
            }
            if (!this->work.load()) {
                if (this->net.Valid()) {
                    this->Accept();
                }
                this->awaitableHandle.Detach();
            }
        }

        void SetHandle(SlokedIOPoller::Handle handle) {
            this->awaitableHandle = std::move(handle);
        }

     private:
        friend class SlaveService;
        class SlaveService : public KgrService {
         public:
            SlaveService(KgrMasterNetServerContext &srv,
                         const std::string &service)
                : srv(srv), service(service) {}

            TaskResult<void> Attach(std::unique_ptr<KgrPipe> pipe) final {
                TaskResultSupplier<void> supplier;
                std::unique_lock lock(this->srv.mtx);
                auto pipeId = this->srv.nextPipeId++;
                lock.unlock();
                this->srv.workers.Enqueue([this, pipeId, supplier,
                                           pipePtr = pipe.release()] {
                    supplier.Wrap([&] {
                        auto rsp = this->srv.net.Invoke(
                            "connect",
                            KgrDictionary{{"pipe", pipeId},
                                          {"service", this->service}});
                        if (rsp.WaitResponse(KgrNetConfig::ResponseTimeout) &&
                            rsp.HasResponse()) {
                            auto remotePipe =
                                rsp.GetResponse().GetResult().AsInt();
                            std::unique_ptr<KgrPipe> pipe{pipePtr};
                            pipe->SetMessageListener([this, pipeId,
                                                      remotePipe] {
                                std::unique_lock lock(this->srv.mtx);
                                if (this->srv.pipes.count(pipeId) != 0) {
                                    auto &pipe = *this->srv.pipes.at(pipeId);
                                    while (!pipe.Empty()) {
                                        this->srv.net.Invoke(
                                            "send", KgrDictionary{
                                                        {"pipe", remotePipe},
                                                        {"data", pipe.Read()}});
                                    }
                                    if (pipe.GetStatus() ==
                                        KgrPipe::Status::Closed) {
                                        this->srv.net.Invoke("close",
                                                             remotePipe);
                                        this->srv.pipes.erase(pipeId);
                                    }
                                }
                            });
                            std::unique_lock lock(this->srv.mtx);
                            while (!pipe->Empty()) {
                                this->srv.net.Invoke(
                                    "send",
                                    KgrDictionary{{"pipe", remotePipe},
                                                  {"data", pipe->Read()}});
                            }
                            if (pipe->GetStatus() == KgrPipe::Status::Closed) {
                                this->srv.net.Invoke("close", remotePipe);
                            } else {
                                this->srv.pipes.emplace(pipeId,
                                                        std::move(pipe));
                            }
                        }
                    });
                });
                return supplier.Result();
            }

         private:
            KgrMasterNetServerContext &srv;
            std::string service;
        };

        void Connect(std::unique_ptr<KgrPipe> pipe,
                     KgrNetInterface::Responder &rsp) {
            std::unique_lock lock(this->mtx);
            auto pipeId = this->nextPipeId++;
            pipe->SetMessageListener([this, pipeId] {
                std::unique_lock lock(this->mtx);
                if (this->frozenPipes.count(pipeId) != 0) {
                    return;
                }
                if (this->pipes.count(pipeId) != 0) {
                    auto &pipe = *this->pipes.at(pipeId);
                    while (!pipe.Empty()) {
                        this->net.Invoke("send",
                                         KgrDictionary{{"pipe", pipeId},
                                                       {"data", pipe.Read()}});
                    }
                    if (pipe.GetStatus() == KgrPipe::Status::Closed) {
                        try {
                            this->net.Invoke("close", pipeId);
                        } catch (const SlokedError &err) {
                            // Ignoring errors in case when socket already
                            // closed
                        }
                        this->frozenPipes.erase(pipeId);
                        this->pipes.erase(pipeId);
                    }
                }
            });
            this->frozenPipes.insert(pipeId);
            this->pipes.emplace(pipeId, std::move(pipe));
            rsp.Result(pipeId);
        }

        void Accept(std::size_t count = 0) {
            if (this->net.Wait(KgrNetConfig::RequestTimeout)) {
                this->net.Receive();
                this->net.Process(count);
                this->lastActivity = std::chrono::system_clock::now();
            }
        }

        bool IsAccessPermitted(const std::string &name) {
            if (this->restrictions == nullptr || this->auth == nullptr) {
                return true;
            }
            std::weak_ptr<SlokedNamedRestrictionAuthority::Account> acc;
            if (this->auth->IsLoggedIn()) {
                acc = this->restrictions->GetRestrictionsByName(
                    this->auth->GetAccount());
            } else {
                acc = this->restrictions->GetDefaultRestrictions();
            }
            if (auto account = acc.lock()) {
                return account->GetAccessRestrictions()->IsAllowed({name});
            } else {
                return false;
            }
        }

        bool IsModificationPermitted(const std::string &name) {
            if (this->restrictions == nullptr || this->auth == nullptr) {
                return true;
            }
            std::weak_ptr<SlokedNamedRestrictionAuthority::Account> acc;
            if (this->auth->IsLoggedIn()) {
                acc = this->restrictions->GetRestrictionsByName(
                    this->auth->GetAccount());
            } else {
                acc = this->restrictions->GetDefaultRestrictions();
            }
            if (auto account = acc.lock()) {
                return account->GetModificationRestrictiions()->IsAllowed(
                    {name});
            } else {
                return false;
            }
        }

        KgrNetInterface net;
        const std::atomic<bool> &work;
        KgrNamedServer &server;
        std::set<std::string> remoteServiceList;
        std::map<int64_t, std::unique_ptr<KgrPipe>> pipes;
        int64_t nextPipeId;
        std::recursive_mutex mtx;
        std::set<int64_t> frozenPipes;
        SlokedCounter<std::size_t>::Handle counterHandle;
        SlokedIOPoller::Handle awaitableHandle;
        std::chrono::system_clock::time_point lastActivity;
        SlokedScopedExecutor workers;
        bool pinged;
        SlokedNamedRestrictionAuthority *restrictions;
        std::unique_ptr<SlokedMasterAuthenticator> auth;
    };

    KgrMasterNetServer::KgrMasterNetServer(
        KgrNamedServer &server, std::unique_ptr<SlokedServerSocket> socket,
        SlokedIOPoller &poll, SlokedExecutor &threadManager,
        SlokedNamedRestrictionAuthority *restrictions,
        SlokedAuthenticatorFactory *authFactory)
        : server(server), srvSocket(std::move(socket)), poll(poll),
          threadManager(threadManager), restrictions(restrictions),
          authFactory(authFactory), work(false) {}

    KgrMasterNetServer::~KgrMasterNetServer() {
        this->Close();
    }

    bool KgrMasterNetServer::IsRunning() const {
        return this->work.load();
    }

    void KgrMasterNetServer::Start() {
        if (this->work.load()) {
            return;
        }
        if (!this->srvSocket->Valid()) {
            throw SlokedError("KgrNetMaster: Closed server socket");
        }
        this->srvSocket->Start();
        this->work = true;
        this->awaiterHandle =
            this->poll.Attach(std::make_unique<Awaitable>(*this));
    }

    void KgrMasterNetServer::Close() {
        if (this->work.exchange(false)) {
            this->awaiterHandle.Detach();
            this->workers.Wait([](auto count) { return count == 0; });
        }
    }

    void KgrMasterNetServer::DetachServices(std::vector<std::string> services) {
        SlokedCounter<std::size_t>::Handle counterHandle(this->workers);
        std::thread([this, counterHandle = std::move(counterHandle),
                     services = std::move(services)] {
            for (const auto &sName : services) {
                this->server.Deregister({sName});
            }
        }).detach();
    }

    KgrMasterNetServer::Awaitable::Awaitable(KgrMasterNetServer &self)
        : self(self) {}

    std::unique_ptr<SlokedIOAwaitable>
        KgrMasterNetServer::Awaitable::GetAwaitable() const {
        return this->self.srvSocket->Awaitable();
    }

    void KgrMasterNetServer::Awaitable::Process(bool success) {
        if (success) {
            auto client =
                this->self.srvSocket->Accept(KgrNetConfig::RequestTimeout);
            if (client) {
                SlokedCounter<std::size_t>::Handle counterHandle(
                    this->self.workers);
                auto ctx = std::make_unique<KgrMasterNetServerContext>(
                    std::move(client), this->self.work,
                    std::move(counterHandle), this->self.threadManager,
                    this->self.server, this->self.restrictions,
                    this->self.authFactory);
                auto &ctx_ref = *ctx;
                auto handle = this->self.poll.Attach(std::move(ctx));
                ctx_ref.SetHandle(std::move(handle));
            }
        }
    }
}  // namespace sloked