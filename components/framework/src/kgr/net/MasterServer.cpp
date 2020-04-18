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
#include "sloked/sched/Pipeline.h"
#include "sloked/sched/ScopedExecutor.h"

using namespace std::chrono_literals;

namespace sloked {

    class KgrMasterNetServerContext : public SlokedIOPoller::Awaitable {
     public:
        KgrMasterNetServerContext(std::unique_ptr<SlokedSocket> socket,
                                  const std::atomic<bool> &work,
                                  SlokedCounter<std::size_t>::Handle counter,
                                  SlokedScheduler &sched,
                                  KgrNamedServer &server,
                                  SlokedNamedRestrictionAuthority *restrictions,
                                  SlokedAuthenticatorFactory *authFactory)
            : net(std::move(socket), sched), work(work), server(server),
              nextPipeId(0), counterHandle(std::move(counter)), pinged{false},
              restrictions(restrictions),
              lifetime(std::make_shared<SlokedStandardLifetime>()) {

            if (authFactory) {
                this->auth = authFactory->NewMaster(this->net.GetEncryption());
            }

            this->lastActivity = std::chrono::system_clock::now();

            this->net.BindMethod("connect", [this](const std::string &method,
                                                   const KgrValue &params,
                                                   auto &rsp) {
                const auto &service = params.AsString();
                std::unique_lock lock(this->mtx);
                if (!this->IsAccessPermitted(service)) {
                    rsp.Error("KgrMasterServer: Access to \'" + service +
                              "\' restricted");
                } else {
                    struct State {
                        State(KgrMasterNetServerContext *self,
                              KgrNetInterface::Responder responder)
                            : self(self), responder(std::move(responder)) {}
                        KgrMasterNetServerContext *self;
                        KgrNetInterface::Responder responder;
                    };

                    static const SlokedAsyncTaskPipeline Pipeline(
                        SlokedTaskPipelineStages::Map(
                            [](const std::shared_ptr<State> &state,
                               std::unique_ptr<KgrPipe> pipe) {
                                if (pipe == nullptr) {
                                    throw SlokedError(
                                        "KgrMasterServer: Pipe can't be null");
                                } else {
                                    state->self->Connect(std::move(pipe),
                                                         state->responder);
                                }
                            }),
                        SlokedTaskPipelineStages::ScanErrors(
                            [](const std::shared_ptr<State> &state,
                               const std::exception_ptr &) {
                                state->responder.Result(false);
                            }),
                        SlokedTaskPipelineStages::ScanCancelled(
                            [](const std::shared_ptr<State> &state,
                               const auto &) {
                                state->responder.Result(false);
                            }));

                    Pipeline(std::make_shared<State>(this, std::move(rsp)),
                             this->server.Connect({service}), this->lifetime);
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
                        struct State {
                            State(KgrMasterNetServerContext *self,
                                  const std::string service,
                                  KgrNetInterface::Responder responder)
                                : self(self), service(service),
                                  responder(std::move(responder)) {}
                            KgrMasterNetServerContext *self;
                            std::string service;
                            KgrNetInterface::Responder responder;
                        };

                        static const SlokedAsyncTaskPipeline Pipeline(
                            SlokedTaskPipelineStages::Map(
                                [](const std::shared_ptr<State> &state,
                                   bool registered) {
                                    if (!registered) {
                                        return state->self->server.Register(
                                            {state->service},
                                            std::make_unique<SlaveService>(
                                                *state->self, state->service));
                                    } else {
                                        return TaskResult<void>::Cancel();
                                    }
                                }),
                            SlokedTaskPipelineStages::Scan(
                                [](const std::shared_ptr<State> &state) {
                                    std::unique_lock lock(state->self->mtx);
                                    state->self->remoteServiceList.insert(
                                        state->service);
                                    state->responder.Result(true);
                                }),
                            SlokedTaskPipelineStages::ScanErrors(
                                [](const std::shared_ptr<State> &state,
                                   const std::exception_ptr &err) {
                                    state->responder.Result(false);
                                }),
                            SlokedTaskPipelineStages::ScanCancelled(
                                [](const std::shared_ptr<State> &state,
                                   const auto &) {
                                    state->responder.Result(false);
                                }));

                        Pipeline(std::make_shared<State>(this, service, rsp),
                                 this->server.Registered({service}),
                                 this->lifetime);
                    }
                });

            this->net.BindMethod("bound", [this](const std::string &method,
                                                 const KgrValue &params,
                                                 auto &rsp) {
                struct State {
                    State(KgrMasterNetServerContext *self,
                          const std::string &service,
                          KgrNetInterface::Responder responder)
                        : self(self), service(service),
                          responder(std::move(responder)) {}
                    KgrMasterNetServerContext *self;
                    std::string service;
                    KgrNetInterface::Responder responder;
                };

                static const SlokedAsyncTaskPipeline Pipeline(
                    SlokedTaskPipelineStages::Map(
                        [](const std::shared_ptr<State> &state,
                           bool registered) {
                            state->responder.Result(
                                registered &&
                                (state->self->IsAccessPermitted(
                                     state->service) ||
                                 state->self->IsModificationPermitted(
                                     state->service)));
                        }),
                    SlokedTaskPipelineStages::ScanErrors(
                        [](const std::shared_ptr<State> &state,
                           const std::exception_ptr &) {
                            state->responder.Result(false);
                        }),
                    SlokedTaskPipelineStages::ScanCancelled(
                        [](const std::shared_ptr<State> &state, const auto &) {
                            state->responder.Result(false);
                        }));

                Pipeline(std::make_shared<State>(this, params.AsString(),
                                                 std::move(rsp)),
                         this->server.Registered({params.AsString()}),
                         this->lifetime);
            });

            this->net.BindMethod(
                "unbind", [this](const std::string &method,
                                 const KgrValue &params, auto &rsp) {
                    const auto &service = params.AsString();
                    if (!this->IsModificationPermitted(service)) {
                        rsp.Error("KgrMasterServer: Modification of \'" +
                                  service + "\' restricted");
                    } else {
                        struct State {
                            State(KgrMasterNetServerContext *self,
                                  const std::string &service,
                                  KgrNetInterface::Responder responder)
                                : self(self), service(service),
                                  responder(std::move(responder)) {}
                            KgrMasterNetServerContext *self;
                            std::string service;
                            KgrNetInterface::Responder responder;
                        };

                        static const SlokedAsyncTaskPipeline Pipeline(
                            SlokedTaskPipelineStages::Scan(
                                [](const std::shared_ptr<State> &state) {
                                    std::unique_lock lock(state->self->mtx);
                                    state->self->remoteServiceList.erase(
                                        state->service);
                                    state->responder.Result(true);
                                }),
                            SlokedTaskPipelineStages::ScanErrors(
                                [](const std::shared_ptr<State> &state,
                                   const std::exception_ptr &) {
                                    state->responder.Result(false);
                                }),
                            SlokedTaskPipelineStages::ScanCancelled(
                                [](const std::shared_ptr<State> &state,
                                   const auto &) {
                                    state->responder.Result(false);
                                }));

                        std::unique_lock lock(this->mtx);
                        if (this->remoteServiceList.count(service) != 0) {
                            lock.unlock();
                            Pipeline(std::make_shared<State>(this, service,
                                                             std::move(rsp)),
                                     this->server.Deregister({service}),
                                     this->lifetime);
                        } else {
                            rsp.Result(false);
                        }
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
                        this->net.Flush();
                        this->auth->FinalizeLogin();
                    }
                } else {
                    rsp.Error("KgrMasterServer: Authentication not supported");
                }
            });
        }

        virtual ~KgrMasterNetServerContext() {
            this->lifetime->Close();
            std::unique_lock lock(this->mtx);
            for (const auto &pipe : this->pipes) {
                pipe.second->Close();
            }
            this->pipes.clear();
            if (this->net.Valid()) {
                this->net.Close();
            }
        }

        std::unique_ptr<SlokedIOAwaitable> GetAwaitable() const final {
            return this->net.Awaitable();
        }

        void Process(bool success) final {
            if (success || this->net.Available() > 0) {
                if (success && this->net.Available() == 0) {
                    this->net.Close();
                } else {
                    this->pinged = false;
                    this->Accept();
                }
            } else if (this->work.load() && this->net.Valid()) {
                auto now = std::chrono::system_clock::now();
                auto idle = now - this->lastActivity;
                if (idle > KgrNetConfig::InactivityThreshold && this->pinged) {
                    // Kicking inactive client out
                    this->net.Close();
                } else if (idle > KgrNetConfig::InactivityTimeout &&
                           !this->pinged) {
                    this->net.Invoke("ping", {});
                    this->pinged = true;
                }
            }
            if (!this->work.load() || !this->net.Valid()) {
                if (this->net.Valid()) {
                    this->Accept();
                }
                this->DeregisterAll();
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
                : srv(srv), service(service),
                  lifetime(std::make_shared<SlokedStandardLifetime>()) {}

            ~SlaveService() {
                this->lifetime->Close();
            }

            TaskResult<void> Attach(std::unique_ptr<KgrPipe> pipe) final {
                struct State {
                    State(SlaveService *self, std::unique_ptr<KgrPipe> pipe,
                          int64_t pipeId)
                        : self(self), pipe(std::move(pipe)), pipeId{pipeId} {}
                    SlaveService *self;
                    std::unique_ptr<KgrPipe> pipe;
                    int64_t pipeId;
                };

                std::unique_lock lock(this->srv.mtx);
                auto pipeId = this->srv.nextPipeId++;
                lock.unlock();

                static const SlokedAsyncTaskPipeline Pipeline(
                    SlokedTaskPipelineStages::Map(
                        [](const std::shared_ptr<State> state,
                           const SlokedNetResponseBroker::Response &response) {
                            auto remotePipe = response.GetResult().AsInt();
                            auto pipe = std::move(state->pipe);
                            auto pipeId = state->pipeId;
                            pipe->SetMessageListener([self = state->self,
                                                      pipeId, remotePipe] {
                                std::unique_lock lock(self->srv.mtx);
                                if (self->srv.pipes.count(pipeId) != 0) {
                                    auto &pipe = *self->srv.pipes.at(pipeId);
                                    while (!pipe.Empty()) {
                                        self->srv.net.Invoke(
                                            "send", KgrDictionary{
                                                        {"pipe", remotePipe},
                                                        {"data", pipe.Read()}});
                                    }
                                    if (pipe.GetStatus() ==
                                        KgrPipe::Status::Closed) {
                                        self->srv.net.Invoke("close",
                                                             remotePipe);
                                        self->srv.pipes.erase(pipeId);
                                    }
                                }
                            });
                            std::unique_lock lock(state->self->srv.mtx);
                            while (!pipe->Empty()) {
                                state->self->srv.net.Invoke(
                                    "send",
                                    KgrDictionary{{"pipe", remotePipe},
                                                  {"data", pipe->Read()}});
                            }
                            if (pipe->GetStatus() == KgrPipe::Status::Closed) {
                                state->self->srv.net.Invoke("close",
                                                            remotePipe);
                            } else {
                                state->self->srv.pipes.emplace(pipeId,
                                                               std::move(pipe));
                            }
                        }));

                return Pipeline(
                    std::make_shared<State>(this, std::move(pipe), pipeId),
                    this->srv.net
                        .Invoke("connect",
                                KgrDictionary{{"pipe", pipeId},
                                              {"service", this->service}})
                        ->Next(),
                    this->lifetime);
            }

         private:
            KgrMasterNetServerContext &srv;
            std::string service;
            std::shared_ptr<SlokedStandardLifetime> lifetime;
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
                            // Ignoring errors in case when socket is already
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
                this->lastActivity = std::chrono::system_clock::now();
                this->net.Receive();
                this->net.Process(count);
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

        void DeregisterAll() {
            static const auto deregisterCallback =
                [](KgrMasterNetServerContext *self) {
                    if (!self->remoteServiceList.empty()) {
                        auto it = self->remoteServiceList.begin();
                        auto res = self->server.Deregister({*it});
                        self->remoteServiceList.erase(it);
                        return res;
                    } else {
                        return TaskResult<void>::Resolve();
                    }
                };
            deregisterCallback(this).Notify([this](const auto &) {
                if (!this->remoteServiceList.empty()) {
                    this->DeregisterAll();
                } else {
                    this->awaitableHandle.Detach();
                }
            });
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
        bool pinged;
        SlokedNamedRestrictionAuthority *restrictions;
        std::unique_ptr<SlokedMasterAuthenticator> auth;
        std::shared_ptr<SlokedStandardLifetime> lifetime;
    };

    KgrMasterNetServer::KgrMasterNetServer(
        KgrNamedServer &server, std::unique_ptr<SlokedServerSocket> socket,
        SlokedIOPoller &poll, SlokedScheduler &sched,
        SlokedNamedRestrictionAuthority *restrictions,
        SlokedAuthenticatorFactory *authFactory)
        : server(server), srvSocket(std::move(socket)), poll(poll),
          sched(sched), restrictions(restrictions), authFactory(authFactory),
          work(false) {}

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
                    std::move(counterHandle), this->self.sched,
                    this->self.server, this->self.restrictions,
                    this->self.authFactory);
                auto &ctx_ref = *ctx;
                auto handle = this->self.poll.Attach(std::move(ctx));
                ctx_ref.SetHandle(std::move(handle));
            }
        }
    }
}  // namespace sloked