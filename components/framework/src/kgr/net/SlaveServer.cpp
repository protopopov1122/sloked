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

#include "sloked/kgr/net/SlaveServer.h"

#include <iostream>
#include <thread>

#include "sloked/core/Base64.h"
#include "sloked/core/Error.h"
#include "sloked/kgr/local/Pipe.h"
#include "sloked/kgr/net/Config.h"

namespace sloked {

    KgrSlaveNetServer::KgrSlaveNetServer(
        std::unique_ptr<SlokedSocket> socket, SlokedIOPoller &poll,
        SlokedScheduler &sched, SlokedAuthenticatorFactory *authFactory)
        : lifetime(std::make_shared<SlokedStandardLifetime>()),
          net(std::move(socket), sched), work(false),
          localServer(rawLocalServer), poll(poll), pinged{false} {

        if (authFactory) {
            this->auth = authFactory->NewSlave(this->net.GetEncryption());
        }

        this->lastActivity = std::chrono::system_clock::now();

        this->net.BindMethod("send", [this](const std::string &method,
                                            const KgrValue &params, auto &rsp) {
            std::unique_lock lock(this->mtx);
            const auto pipeId = params.AsDictionary()["pipe"].AsInt();
            const auto &data = params.AsDictionary()["data"];
            if (this->pipes.count(pipeId) != 0) {
                this->pipes.at(pipeId)->Write(KgrValue{data});
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        });

        this->net.BindMethod(
            "close", [this](const std::string &method, const KgrValue &params,
                            auto &rsp) {
                std::unique_lock lock(this->mtx);
                const auto pipeId = params.AsInt();
                if (this->pipes.count(pipeId) != 0) {
                    this->pipes.erase(pipeId);
                }
            });

        this->net.BindMethod("connect", [this](const std::string &method,
                                               const KgrValue &params,
                                               auto &rsp) {
            std::unique_lock lock(this->mtx);
            const auto &service = params.AsDictionary()["service"].AsString();
            auto remotePipe = params.AsDictionary()["pipe"].AsInt();
            auto pipe =
                std::move(this->localServer.Connect({service}).UnwrapWait());
            if (pipe == nullptr) {
                throw SlokedError("KgrSlaveServer: Pipe can't be null");
            }
            pipe->SetMessageListener([this, remotePipe] {
                std::unique_lock lock(this->mtx);
                auto &pipe = *this->pipes.at(remotePipe);
                while (!pipe.Empty()) {
                    this->net.Invoke("send",
                                     KgrDictionary{{"pipe", remotePipe},
                                                   {"data", pipe.Read()}});
                }
                if (pipe.GetStatus() == KgrPipe::Status::Closed) {
                    this->net.Invoke("close", remotePipe);
                    this->pipes.erase(remotePipe);
                }
            });
            rsp.Result(remotePipe);
            while (!pipe->Empty()) {
                this->net.Invoke("send", KgrDictionary{{"pipe", remotePipe},
                                                       {"data", pipe->Read()}});
            }
            if (pipe->GetStatus() == KgrPipe::Status::Closed) {
                this->net.Invoke("close", remotePipe);
                this->pipes.erase(remotePipe);
            }
            this->pipes.emplace(remotePipe, std::move(pipe));
        });

        this->net.BindMethod(
            "ping", [](const std::string &method, const KgrValue &params,
                       auto &rsp) { rsp.Result("pong"); });
    }

    KgrSlaveNetServer::~KgrSlaveNetServer() {
        this->Close();
    }

    bool KgrSlaveNetServer::IsRunning() const {
        return this->work.load();
    }

    void KgrSlaveNetServer::Start() {
        std::unique_lock lock(this->mtx);
        if (this->work.exchange(true)) {
            return;
        }
        this->awaitableHandle =
            this->poll.Attach(std::make_unique<Awaitable>(*this));
    }

    void KgrSlaveNetServer::Close() {
        if (this->work.exchange(false)) {
            this->lifetime->Close();
            this->awaitableHandle.Detach();
            for (const auto &pipe : this->pipes) {
                pipe.second->Close();
            }
            if (this->net.Valid()) {
                this->net.Close();
            }
        }
    }

    TaskResult<std::unique_ptr<KgrPipe>> KgrSlaveNetServer::Connect(
        const SlokedPath &service) {
        TaskResultSupplier<std::unique_ptr<KgrPipe>> supplier;
        supplier.Wrap([&]() -> std::unique_ptr<KgrPipe> {
            if (!this->work.load()) {
                return nullptr;
            }
            auto rsp = this->net.Invoke("connect", service.ToString())->Next();
            if (rsp.WaitFor(KgrNetConfig::ResponseTimeout) ==
                    TaskResultStatus::Ready &&
                this->work.load()) {
                const auto &res = rsp.Unwrap();
                if (res.HasResult()) {
                    std::unique_lock lock(this->mtx);
                    int64_t pipeId = res.GetResult().AsInt();
                    auto [pipe1, pipe2] = KgrLocalPipe::Make();
                    pipe1->SetMessageListener([this, pipeId] {
                        std::unique_lock lock(this->mtx);
                        if (this->pipes.count(pipeId) == 0) {
                            return;
                        }
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
                    });
                    this->pipes.emplace(pipeId, std::move(pipe1));
                    this->net.Invoke("activate", pipeId);
                    return std::move(pipe2);
                } else {
                    throw SlokedError(res.GetError().AsString());
                }
            }
            return nullptr;
        });
        return supplier.Result();
    }

    KgrSlaveNetServer::Connector KgrSlaveNetServer::GetConnector(
        const SlokedPath &service) {
        return [this, service] { return this->Connect(service); };
    }

    TaskResult<void> KgrSlaveNetServer::Register(
        const SlokedPath &serviceName, std::unique_ptr<KgrService> service) {
        TaskResultSupplier<void> supplier;
        supplier.Catch([&] {
            auto id = serviceName.ToString();
            auto rsp = this->localServer.Registered(serviceName);
            rsp.Notify(
                [this, supplier, id, serviceName,
                 servicePtr = service.release()](const auto &result) mutable {
                    std::unique_ptr<KgrService> service{servicePtr};
                    servicePtr = nullptr;
                    supplier.Catch([&] {
                        if (result.State() == TaskResultStatus::Ready &&
                            !result.Unwrap()) {
                            this->localServer
                                .Register(serviceName, std::move(service))
                                .Notify(
                                    [this, supplier, id,
                                     serviceName](const auto &) {
                                        supplier.Wrap([&] {
                                            auto rsp =
                                                this->net.Invoke("bind", id)
                                                    ->Next();
                                            if (!(rsp.WaitFor(
                                                      KgrNetConfig::
                                                          ResponseTimeout) ==
                                                      TaskResultStatus::Ready &&
                                                  this->work.load()) ||
                                                !rsp.Unwrap()
                                                     .GetResult()
                                                     .AsBoolean()) {
                                                this->localServer.Deregister(
                                                    serviceName);
                                                throw SlokedError(
                                                    "KgrSlaveServer: Error "
                                                    "registering "
                                                    "service " +
                                                    id);
                                            }
                                        });
                                    },
                                    this->lifetime);
                        } else {
                            throw SlokedError("KgrSlaveServer: Service " + id +
                                              " already registered");
                        }
                    });
                },
                this->lifetime);
        });
        return supplier.Result();
    }

    TaskResult<bool> KgrSlaveNetServer::Registered(const SlokedPath &service) {
        TaskResultSupplier<bool> supplier;
        supplier.Catch([&] {
            this->localServer.Registered(service).Notify(
                [this, supplier, service](const auto &result) {
                    supplier.Wrap([&] {
                        if (result.Unwrap()) {
                            return true;
                        } else {
                            auto rsp =
                                this->net.Invoke("bound", service.ToString())
                                    ->Next();
                            return rsp.WaitFor(KgrNetConfig::ResponseTimeout) ==
                                       TaskResultStatus::Ready &&
                                   this->work.load() &&
                                   rsp.Unwrap().HasResult() &&
                                   rsp.Unwrap().GetResult().AsBoolean();
                        }
                    });
                },
                this->lifetime);
        });
        return supplier.Result();
    }

    TaskResult<void> KgrSlaveNetServer::Deregister(const SlokedPath &service) {
        TaskResultSupplier<void> supplier;
        auto id = service.ToString();
        this->localServer.Registered(service).Notify(
            [this, service, supplier, id](const auto &result) {
                supplier.Wrap([&] {
                    if (result.State() == TaskResultStatus::Ready &&
                        result.Unwrap()) {
                        this->localServer.Deregister(service);
                        auto rsp = this->net.Invoke("unbind", id)->Next();
                        if (!(rsp.WaitFor(KgrNetConfig::ResponseTimeout) ==
                                  TaskResultStatus::Ready &&
                              this->work.load()) ||
                            !rsp.Unwrap().GetResult().AsBoolean()) {
                            throw SlokedError(
                                "KgrSlaveServer: Error deregistering " + id);
                        }
                    } else {
                        throw SlokedError("KgrSlaveServer: Service " + id +
                                          " not registered");
                    }
                });
            },
            this->lifetime);
        return supplier.Result();
    }

    void KgrSlaveNetServer::Authorize(const std::string &account) {
        if (this->auth == nullptr) {
            throw SlokedError("KgrSlaveServer: Authenticator not defined");
        }
        // Sending login request
        auto loginRequest = this->net.Invoke("auth-request", {})->Next();
        if (!(loginRequest.WaitFor(KgrNetConfig::ResponseTimeout) ==
                  TaskResultStatus::Ready &&
              this->work.load())) {
            throw SlokedError("KgrSlaveServer: Error requesting login for \'" +
                              account + "\'");
        }
        auto nonce = static_cast<SlokedSlaveAuthenticator::Challenge>(
            loginRequest.Unwrap().GetResult().AsDictionary()["nonce"].AsInt());
        auto result = this->auth->InitiateLogin(account, nonce);

        // Sending result
        auto loginResponse =
            this->net
                .Invoke("auth-response",
                        KgrDictionary{{"id", account}, {"result", result}})
                ->Next();
        if (!(loginResponse.WaitFor(KgrNetConfig::ResponseTimeout) ==
                  TaskResultStatus::Ready &&
              this->work.load() &&
              loginResponse.Unwrap().GetResult().AsBoolean())) {
            throw SlokedError("KgrSlaveServer: Error requesting login for \'" +
                              account + "\'");
        }
        this->auth->ContinueLogin(account);
        this->net.Flush();
        this->auth->FinalizeLogin();
    }

    void KgrSlaveNetServer::Accept() {
        if (this->net.Wait(KgrNetConfig::RequestTimeout)) {
            this->net.Receive();
            this->net.Process();
            this->lastActivity = std::chrono::system_clock::now();
        }
    }

    void KgrSlaveNetServer::Ping() {
        auto now = std::chrono::system_clock::now();
        auto idle = now - this->lastActivity;
        if (idle > KgrNetConfig::InactivityThreshold && this->pinged) {
            throw SlokedError("KgrSlaveServer: Connection inactive for " +
                              std::to_string(idle.count()) + " ns");
        } else if (idle > KgrNetConfig::InactivityTimeout && !this->pinged) {
            this->net.Invoke("ping", {});
            this->pinged = true;
        }
    }

    KgrSlaveNetServer::Awaitable::Awaitable(KgrSlaveNetServer &self)
        : self(self) {}

    std::unique_ptr<SlokedIOAwaitable>
        KgrSlaveNetServer::Awaitable::GetAwaitable() const {
        return this->self.net.Awaitable();
    }

    void KgrSlaveNetServer::Awaitable::Process(bool success) {
        if (success || this->self.net.Available() > 0) {
            this->self.pinged = false;
            this->self.Accept();
        } else if (this->self.net.Valid()) {
            this->self.Ping();
        }
        if (!this->self.net.Valid()) {
            this->self.Close();
        }
    }
}  // namespace sloked