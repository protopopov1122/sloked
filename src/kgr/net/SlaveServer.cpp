/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/kgr/net/SlaveServer.h"
#include "sloked/kgr/local/Pipe.h"
#include "sloked/core/Error.h"
#include "sloked/kgr/net/Config.h"
#include "sloked/core/Base64.h"
#include <thread>
#include <iostream>

namespace sloked {

    KgrSlaveNetServer::KgrSlaveNetServer(std::unique_ptr<SlokedSocket> socket, SlokedIOPoller &poll, SlokedAuthenticatorFactory *authFactory)
        : net(std::move(socket)), work(false), localServer(rawLocalServer), poll(poll), pinged{false} {

        if (authFactory) {
            this->auth = authFactory->NewSlave(this->net.GetEncryption());
        }

        this->lastActivity = std::chrono::system_clock::now();

        this->net.BindMethod("send", [this](const std::string &method, const KgrValue &params, auto &rsp) {
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

        this->net.BindMethod("close", [this](const std::string &method, const KgrValue &params, auto &rsp) {
            std::unique_lock lock(this->mtx);
            const auto pipeId = params.AsInt();
            if (this->pipes.count(pipeId) != 0) {
                this->pipes.erase(pipeId);
            }
        }); 

        this->net.BindMethod("connect", [this](const std::string &method, const KgrValue &params, auto &rsp) {
            std::unique_lock lock(this->mtx);
            const auto &service = params.AsDictionary()["service"].AsString();
            auto remotePipe = params.AsDictionary()["pipe"].AsInt();
            auto pipe = this->localServer.Connect(service);
            if (pipe == nullptr) {
                throw SlokedError("KgrSlaveServer: Pipe can't be null");
            }
            pipe->SetMessageListener([this, remotePipe] {
                std::unique_lock lock(this->mtx);
                auto &pipe = *this->pipes.at(remotePipe);
                while (!pipe.Empty()) {
                    this->net.Invoke("send", KgrDictionary {
                        { "pipe", remotePipe },
                        { "data", pipe.Read() }
                    });
                }
                if (pipe.GetStatus() == KgrPipe::Status::Closed) {
                    this->net.Invoke("close", remotePipe);
                    this->pipes.erase(remotePipe);
                }
            });
            rsp.Result(remotePipe);
            while (!pipe->Empty()) {
                this->net.Invoke("send", KgrDictionary {
                    { "pipe", remotePipe },
                    { "data", pipe->Read() }
                });
            }
            if (pipe->GetStatus() == KgrPipe::Status::Closed) {
                this->net.Invoke("close", remotePipe);
                this->pipes.erase(remotePipe);
            }
            this->pipes.emplace(remotePipe, std::move(pipe));
        });

        this->net.BindMethod("ping", [](const std::string &method, const KgrValue &params, auto &rsp) {
            rsp.Result("pong");
        });
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
        this->awaitableHandle = this->poll.Attach(std::make_unique<Awaitable>(*this));
    }

    void KgrSlaveNetServer::Close() {
        if (this->work.exchange(false)) {
            this->awaitableHandle.Detach();
            for (const auto &pipe : this->pipes) {
                pipe.second->Close();
            }
            if (this->net.Valid()) {
                this->net.Close();
            }
        }
    }

    std::unique_ptr<KgrPipe> KgrSlaveNetServer::Connect(const std::string &service) {
        if (!this->work.load()) {
            return nullptr;
        }
        auto rsp = this->net.Invoke("connect", service);
        if (rsp.WaitResponse(KgrNetConfig::ResponseTimeout) && this->work.load()) {
            const auto &res = rsp.GetResponse();
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
                        this->net.Invoke("send", KgrDictionary {
                            { "pipe", pipeId },
                            { "data", pipe.Read() }
                        });
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
                std::terminate();
                throw SlokedError(res.GetError().AsString());
            }
        }
        return nullptr;
    }
    
    KgrSlaveNetServer::Connector KgrSlaveNetServer::GetConnector(const std::string &service) {
        return [this, service] {
            return this->Connect(service);
        };
    }
    
    void KgrSlaveNetServer::Register(const std::string &serviceName, std::unique_ptr<KgrService> service) {
        if (!this->localServer.Registered(serviceName)) {
            this->localServer.Register(serviceName, std::move(service));
            auto rsp = this->net.Invoke("bind", serviceName);
            if (!(rsp.WaitResponse(KgrNetConfig::ResponseTimeout) && this->work.load()) ||
                 !rsp.GetResponse().GetResult().AsBoolean()) {
                this->localServer.Deregister(serviceName);
                throw SlokedError("KgrSlaveServer: Error registering service " + serviceName);
            }
        } else {
            throw SlokedError("KgrSlaveServer: Service " + serviceName + " already registered");
        }
    }

    bool KgrSlaveNetServer::Registered(const std::string &service) {
        if (this->localServer.Registered(service)) {
            return true;
        } else {
            auto rsp = this->net.Invoke("bound", service);
            return rsp.WaitResponse(KgrNetConfig::ResponseTimeout) && this->work.load() &&
                rsp.HasResponse() && rsp.GetResponse().GetResult().AsBoolean();
        }
    }

    void KgrSlaveNetServer::Deregister(const std::string &service) {
        if (this->localServer.Registered(service)) {
            this->localServer.Deregister(service);
            auto rsp = this->net.Invoke("unbind", service);
            if (!(rsp.WaitResponse(KgrNetConfig::ResponseTimeout) && this->work.load()) ||
                !rsp.GetResponse().GetResult().AsBoolean()) {
                throw SlokedError("KgrSlaveServer: Error deregistering " + service);
            }
        } else {
            throw SlokedError("KgrSlaveServer: Service " + service + " not registered");
        }
    }

    void KgrSlaveNetServer::Authorize(const std::string &account) {
        if (this->auth == nullptr) {
            throw SlokedError("KgrSlaveServer: Authenticator not defined");
        }
        // Sending login request
        auto loginRequest = this->net.Invoke("auth-request", {});
        if (!(loginRequest.WaitResponse(KgrNetConfig::ResponseTimeout) && this->work.load())) {
            throw SlokedError("KgrSlaveServer: Error requesting login for \'" + account + "\'");
        }
        auto nonce = static_cast<SlokedSlaveAuthenticator::Challenge>(loginRequest.GetResponse().GetResult().AsDictionary()["nonce"].AsInt());
        auto result = this->auth->InitiateLogin(account, nonce);
        
        // Sending result
        auto loginResponse = this->net.Invoke("auth-response", KgrDictionary {
            { "id", account },
            { "result", result }
        });
        if (!(loginResponse.WaitResponse(KgrNetConfig::ResponseTimeout) && this->work.load() &&
            loginResponse.GetResponse().GetResult().AsBoolean())) {
            throw SlokedError("KgrSlaveServer: Error requesting login for \'" + account + "\'");
        }
        this->auth->ContinueLogin(account);
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
            throw SlokedError("KgrSlaveServer: Connection inactive for " + std::to_string(idle.count()) + " ns");
        } else if (idle > KgrNetConfig::InactivityTimeout && !this->pinged) {
            this->net.Invoke("ping", {});
            this->pinged = true;
        }
    }

    KgrSlaveNetServer::Awaitable::Awaitable(KgrSlaveNetServer &self)
        : self(self) {}

    std::unique_ptr<SlokedIOAwaitable> KgrSlaveNetServer::Awaitable::GetAwaitable() const {
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
}