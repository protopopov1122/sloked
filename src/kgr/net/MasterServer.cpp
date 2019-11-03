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

#include "sloked/kgr/net/MasterServer.h"
#include "sloked/core/Error.h"
#include "sloked/kgr/net/Interface.h"
#include <thread>
#include <cstring>
#include <set>
#include <algorithm>

namespace sloked {

    class KgrMasterNetServerContext {
     public:
        KgrMasterNetServerContext(std::unique_ptr<SlokedSocket> socket, const std::atomic<bool> &work, KgrNamedServer &server)
            : net(std::move(socket)), work(work), server(server), nextPipeId(0) {

            this->net.BindMethod("connect", [this](const std::string &method, const KgrValue &params, auto &rsp) {
                const auto &service = params.AsString();
                auto pipe = this->server.Connect(service);
                if (pipe == nullptr) {
                    throw SlokedError("KgrMasterServer: Pipe can't be null");
                }
                std::unique_lock lock(this->mtx);
                auto pipeId = this->nextPipeId++;
                pipe->SetMessageListener([this, pipeId] {
                    std::unique_lock lock(this->mtx);
                    if (this->frozenPipes.count(pipeId) != 0) {
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
                        try {
                            this->net.Invoke("close", pipeId);
                        } catch (const SlokedError &err) {
                            // Ignoring errors in case when socket already closed
                        }
                        this->frozenPipes.erase(pipeId);
                        this->pipes.erase(pipeId);
                    }
                });
                this->frozenPipes.insert(pipeId);
                this->pipes.emplace(pipeId, std::move(pipe));
                rsp.Result(pipeId);
            });

            this->net.BindMethod("activate", [this](const std::string &method, const KgrValue &params, auto &rsp) {
                std::unique_lock lock(this->mtx);
                const auto pipeId = params.AsInt();
                if (this->frozenPipes.count(pipeId) != 0) {
                    this->frozenPipes.erase(pipeId);
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
                }
            });

            this->net.BindMethod("send", [this](const std::string &method, const KgrValue &params, auto &rsp) {
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

            this->net.BindMethod("close", [this](const std::string &method, const KgrValue &params, auto &rsp) {
                std::unique_lock lock(this->mtx);
                const auto pipeId = params.AsInt();
                if (this->pipes.count(pipeId)) {
                    auto &pipe = *this->pipes.at(pipeId);
                    pipe.Close();
                    this->pipes.erase(pipeId);
                    rsp.Result(true);
                } else {
                    rsp.Result(false);
                }
            });

            this->net.BindMethod("bind", [this](const std::string &method, const KgrValue &params, auto &rsp) {
                std::unique_lock lock(this->mtx);
                const auto &service = params.AsString();
                if (!this->server.Registered(service)) {
                    this->server.Register(service, std::make_unique<SlaveService>(*this, service));
                    this->remoteServices.push_back(service);
                    rsp.Result(true);
                } else {
                    rsp.Result(false);
                }
            });

            this->net.BindMethod("bound", [this](const std::string &method, const KgrValue &params, auto &rsp) {
                std::unique_lock lock(this->mtx);
                const auto &service = params.AsString();
                rsp.Result(this->server.Registered(service));
            });

            this->net.BindMethod("unbind", [this](const std::string &method, const KgrValue &params, auto &rsp) {
                std::unique_lock lock(this->mtx);
                const auto &service = params.AsString();
                if (this->server.Registered(service)) {
                    this->server.Deregister(service);
                    this->remoteServices.erase(std::remove(this->remoteServices.begin(), this->remoteServices.end(), service));
                    rsp.Result(true);
                } else {
                    rsp.Result(false);
                }
            });
        }

        void Start() {
            while (work.load() && this->net.Valid()) {
                this->Accept();
            }
            this->Accept();
            for (const auto &pipe : this->pipes) {
                pipe.second->Close();
            }
            for (const auto &service : this->remoteServices) {
                this->server.Deregister(service);
            }
            if (this->net.Valid()) {
                this->net.Close();
            }
        }

     private:
        friend class SlaveService;
        class SlaveService : public KgrService {
         public:
            SlaveService(KgrMasterNetServerContext &srv, const std::string &service)
                : srv(srv), service(service) {}

            bool Attach(std::unique_ptr<KgrPipe> pipe) override {
                try {
                    std::unique_lock lock(this->srv.mtx);
                    auto pipeId = this->srv.nextPipeId++;
                    auto rsp = this->srv.net.Invoke("connect", KgrDictionary {
                        { "pipe", pipeId },
                        { "service", this->service }
                    });
                    while (!rsp.WaitResponse(50) && this->srv.work.load()) {
                        this->srv.Accept(1);
                    }
                    if (rsp.HasResponse()) {
                        auto remotePipe = rsp.GetResponse().GetResult().AsInt();
                        pipe->SetMessageListener([this, pipeId, remotePipe] {
                            std::unique_lock lock(this->srv.mtx);
                            auto &pipe = *this->srv.pipes.at(pipeId);
                            while (!pipe.Empty()) {
                                this->srv.net.Invoke("send", KgrDictionary {
                                    { "pipe", remotePipe },
                                    { "data", pipe.Read() }
                                });
                            }
                            if (pipe.GetStatus() == KgrPipe::Status::Closed) {
                                this->srv.net.Invoke("close", remotePipe);
                                this->srv.pipes.erase(pipeId);
                            }
                        });
                        this->srv.pipes.emplace(pipeId, std::move(pipe));
                        return true;
                    } else {
                        return false;
                    }
                } catch (const SlokedError &err) {
                    return false;
                }
            }

         private:
            KgrMasterNetServerContext &srv;
            std::string service;
        };

        void Accept(std::size_t count = 0) {
            constexpr long Timeout = 50;
            if (this->net.Wait(Timeout)) {
                this->net.Receive();
                this->net.Process(count);
            }
        }

        KgrNetInterface net;
        const std::atomic<bool> &work;
        KgrNamedServer &server;
        std::map<int64_t, std::unique_ptr<KgrPipe>> pipes;
        int64_t nextPipeId;
        std::recursive_mutex mtx;
        std::set<int64_t> frozenPipes;
        std::vector<std::string> remoteServices;
    };

    KgrMasterNetServer::KgrMasterNetServer(KgrNamedServer &server, std::unique_ptr<SlokedServerSocket> socket)
        : server(server), srvSocket(std::move(socket)), work(false) {}
        
    bool KgrMasterNetServer::IsRunning() const {
        return this->work.load();
    }

    void KgrMasterNetServer::Start() {
        std::unique_lock<std::mutex> lock(this->mtx);
        if (this->work.load()) {
            return;
        }
        if (!this->srvSocket->Valid()) {
            throw SlokedError("KgrNetMaster: Closed server socket");
        }
        auto th = std::thread([this] {
            this->srvSocket->Start();
            std::unique_lock<std::mutex> lock(this->mtx);
            this->work = true;
            this->cv.notify_all();
            lock.unlock();
            SlokedCounter<std::size_t>::Handle workerCounter(this->workers);
            while (this->work.load()) {
                constexpr long Timeout = 50;
                auto client = this->srvSocket->Accept(Timeout);
                if (client) {
                    SlokedCounter<std::size_t>::Handle counterHandle(this->workers);
                    std::thread([this, counter = std::move(counterHandle), socket = std::move(client)]() mutable {
                        KgrMasterNetServerContext ctx(std::move(socket), this->work, this->server);
                        ctx.Start();
                    }).detach();
                }
            }
            this->srvSocket->Close();
        });
        th.detach();
        this->cv.wait(lock, [this] { return this->work.load(); });
    }

    void KgrMasterNetServer::Stop() {
        if (!this->work.load()) {
            return;
        }
        this->work = false;
        this->workers.Wait([](auto count) { return count == 0; });
    }
}