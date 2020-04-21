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
#include "sloked/sched/Pipeline.h"

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
            struct State {
                State(KgrSlaveNetServer *self, int64_t remotePipe,
                      KgrNetInterface::Responder responder)
                    : self(self), remotePipe(remotePipe),
                      responder(std::move(responder)) {}

                KgrSlaveNetServer *self;
                int64_t remotePipe;
                KgrNetInterface::Responder responder;
            };

            static const SlokedAsyncTaskPipeline Pipeline(
                SlokedTaskPipelineStages::Map(
                    [](const std::shared_ptr<State> &state,
                       std::unique_ptr<KgrPipe> pipe) {
                        std::unique_lock lock(state->self->mtx);
                        if (pipe == nullptr) {
                            throw SlokedError(
                                "KgrSlaveServer: Pipe can't be null");
                        }
                        pipe->SetMessageListener([self = state->self,
                                                  remotePipe =
                                                      state->remotePipe] {
                            std::unique_lock lock(self->mtx);
                            auto &pipe = *self->pipes.at(remotePipe);
                            while (!pipe.Empty()) {
                                self->net.Invoke(
                                    "send",
                                    KgrDictionary{{"pipe", remotePipe},
                                                  {"data", pipe.Read()}});
                            }
                            if (pipe.GetStatus() == KgrPipe::Status::Closed) {
                                self->net.Invoke("close", remotePipe);
                                self->pipes.erase(remotePipe);
                            }
                        });
                        state->responder.Result(state->remotePipe);
                        while (!pipe->Empty()) {
                            state->self->net.Invoke(
                                "send",
                                KgrDictionary{{"pipe", state->remotePipe},
                                              {"data", pipe->Read()}});
                        }
                        if (pipe->GetStatus() == KgrPipe::Status::Closed) {
                            state->self->net.Invoke("close", state->remotePipe);
                            state->self->pipes.erase(state->remotePipe);
                        }
                        state->self->pipes.emplace(state->remotePipe,
                                                   std::move(pipe));
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

            const auto &service = params.AsDictionary()["service"].AsString();
            auto remotePipe = params.AsDictionary()["pipe"].AsInt();
            Pipeline(std::make_shared<State>(this, remotePipe, std::move(rsp)),
                     this->localServer.Connect({service}), this->lifetime);
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
        struct State {
            State(KgrSlaveNetServer *self, const SlokedPath &service)
                : self(self), service(service) {}
            KgrSlaveNetServer *self;
            SlokedPath service;
        };
        if (!this->work.load()) {
            return TaskResult<std::unique_ptr<KgrPipe>>::Resolve(nullptr);
        }

        static const SlokedAsyncTaskPipeline Pipeline(
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> state,
                   const SlokedNetResponseBroker::Response &res) {
                    if (res.HasResult()) {
                        std::unique_lock lock(state->self->mtx);
                        int64_t pipeId = res.GetResult().AsInt();
                        auto [pipe1, pipe2] = KgrLocalPipe::Make();
                        pipe1->SetMessageListener([self = state->self, pipeId] {
                            std::unique_lock lock(self->mtx);
                            if (self->pipes.count(pipeId) == 0) {
                                return;
                            }
                            auto &pipe = *self->pipes.at(pipeId);
                            while (!pipe.Empty()) {
                                self->net.Invoke(
                                    "send",
                                    KgrDictionary{{"pipe", pipeId},
                                                  {"data", pipe.Read()}});
                            }
                            if (pipe.GetStatus() == KgrPipe::Status::Closed) {
                                self->net.Invoke("close", pipeId);
                                self->pipes.erase(pipeId);
                            }
                        });
                        state->self->pipes.emplace(pipeId, std::move(pipe1));
                        state->self->net.Invoke("activate", pipeId);
                        return std::move(pipe2);
                    } else {
                        throw SlokedError(res.GetError().AsString());
                    }
                }));
        return Pipeline(std::make_shared<State>(this, service),
                        this->net.Invoke("connect", service.ToString())->Next(),
                        this->lifetime);
    }

    KgrSlaveNetServer::Connector KgrSlaveNetServer::GetConnector(
        const SlokedPath &service) {
        return [this, service] { return this->Connect(service); };
    }

    TaskResult<void> KgrSlaveNetServer::Register(
        const SlokedPath &serviceName, std::unique_ptr<KgrService> service) {
        struct State {
            State(KgrSlaveNetServer *self, const SlokedPath &serviceName,
                  std::unique_ptr<KgrService> service)
                : self(self), serviceName(serviceName),
                  service(std::move(service)) {}

            KgrSlaveNetServer *self;
            SlokedPath serviceName;
            std::unique_ptr<KgrService> service;
        };

        static const SlokedAsyncTaskPipeline Pipeline(
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state, bool registered) {
                    if (!registered) {
                        return state->self->localServer.Register(
                            state->serviceName, std::move(state->service));
                    } else {
                        throw SlokedError("KgrSlaveServer: Service " +
                                          state->serviceName.ToString() +
                                          " already registered");
                    }
                }),
            SlokedTaskPipelineStages::MapCancelled(
                [](const std::shared_ptr<State> &state) {
                    throw SlokedError("KgrSlaveServer: Service " +
                                      state->serviceName.ToString() +
                                      " already registered");
                }),
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state) {
                    return state->self->net
                        .Invoke("bind", state->serviceName.ToString())
                        ->Next();
                    ;
                }),
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state,
                   const SlokedNetResponseBroker::Response &rsp) {
                    if (!rsp.HasResult() || !rsp.GetResult().AsBoolean()) {
                        state->self->localServer.Deregister(state->serviceName);
                        throw SlokedError("KgrSlaveServer: Error "
                                          "registering "
                                          "service " +
                                          state->serviceName.ToString());
                    }
                }));

        return Pipeline(
            std::make_shared<State>(this, serviceName, std::move(service)),
            this->localServer.Registered(serviceName), this->lifetime);
    }

    TaskResult<bool> KgrSlaveNetServer::Registered(const SlokedPath &service) {
        struct State {
            State(KgrSlaveNetServer *self, const SlokedPath &service,
                  std::weak_ptr<SlokedLifetime> lifetime)
                : self(self), service(service), lifetime(std::move(lifetime)) {}
            KgrSlaveNetServer *self;
            SlokedPath service;
            std::weak_ptr<SlokedLifetime> lifetime;
        };

        static const SlokedAsyncTaskPipeline NetPipeline(
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state,
                   const SlokedNetResponseBroker::Response &response) {
                    return state->self->work.load() && response.HasResult() &&
                           response.GetResult().AsBoolean();
                }),
            SlokedTaskPipelineStages::Catch(
                [](const std::shared_ptr<State> &state,
                   const std::exception_ptr &err) { return false; }),
            SlokedTaskPipelineStages::MapCancelled(
                [](const std::shared_ptr<State> &state) { return false; }));

        static const SlokedAsyncTaskPipeline MainPipeline(
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state, bool registered) {
                    if (registered) {
                        return TaskResult<bool>::Resolve(true);
                    } else {
                        return NetPipeline(
                            state,
                            state->self->net
                                .Invoke("bound", state->service.ToString())
                                ->Next(),
                            state->lifetime);
                    }
                }));

        return MainPipeline(
            std::make_shared<State>(this, service, this->lifetime),
            this->localServer.Registered(service), this->lifetime);
    }

    TaskResult<void> KgrSlaveNetServer::Deregister(const SlokedPath &service) {
        struct State {
            State(KgrSlaveNetServer *self, const SlokedPath &service)
                : self(self), service(service) {}
            KgrSlaveNetServer *self;
            SlokedPath service;
        };

        static const SlokedAsyncTaskPipeline Pipeline(
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state, bool registered) {
                    if (registered) {
                        return state->self->localServer.Deregister(
                            state->service);
                    } else {
                        throw SlokedError("KgrSlaveServer: Service " +
                                          state->service.ToString() +
                                          " not registered");
                    }
                }),
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state) {
                    return state->self->net
                        .Invoke("unbind", state->service.ToString())
                        ->Next();
                }),
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state,
                   const SlokedNetResponseBroker::Response &response) {
                    if (!state->self->work.load() || !response.HasResult() ||
                        !response.GetResult().AsBoolean()) {
                        throw SlokedError(
                            "KgrSlaveServer: Error deregistering " +
                            state->service.ToString());
                    }
                }),
            SlokedTaskPipelineStages::MapCancelled(
                [](const std::shared_ptr<State> &state) {
                    throw SlokedError("KgrSlaveServer: Service " +
                                      state->service.ToString() +
                                      " not registered");
                }));

        return Pipeline(std::make_shared<State>(this, service),
                        this->localServer.Registered(service), this->lifetime);
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