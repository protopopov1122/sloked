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
#include <iostream>

namespace sloked {
    enum class ContextState {
        Initial,
        Loop,
        Destroy
    };

    class KgrMasterNetServerContext {
     public:
        KgrMasterNetServerContext(std::unique_ptr<SlokedSocket> socket, const std::atomic<bool> &work, KgrNamedServer &server)
            : net(std::move(socket)), work(work), server(server), nextPipeId(0) {

            this->net.BindMethod("connect", [this](const std::string &method, const KgrValue &params, auto &rsp) {
                std::unique_lock<std::mutex> lock(this->mtx);
                const auto &service = params.AsString();
                auto pipe = this->server.Connect(service);
                auto pipeId = this->nextPipeId++;
                pipe->SetMessageListener([this, pipeId] {
                    std::unique_lock<std::mutex> lock(this->mtx);
                    auto &pipe = *this->pipes.at(pipeId);
                    while (!pipe.Empty()) {
                        this->net.Invoke("send", KgrDictionary {
                            { "pipe", pipeId },
                            { "data", pipe.Read() }
                        });
                    }
                });
                this->pipes.emplace(pipeId, std::move(pipe));
                rsp.Result(pipeId);
            });

            this->net.BindMethod("send", [this](const std::string &method, const KgrValue &params, auto &rsp) {
                std::unique_lock<std::mutex> lock(this->mtx);
                const auto pipeId = params.AsDictionary()["pipe"].AsInt();
                const auto &data = params.AsDictionary()["data"];
                if (this->pipes.count(pipeId)) {
                    this->pipes.at(pipeId)->Write(KgrValue{data});
                    rsp.Result(true);
                } else {
                    rsp.Result(false);
                }
            });

            this->net.BindMethod("close", [this](const std::string &method, const KgrValue &params, auto &rsp) {
                std::unique_lock<std::mutex> lock(this->mtx);
                const auto pipeId = params.AsInt();
                if (this->pipes.count(pipeId)) {
                    this->pipes.erase(pipeId);
                    rsp.Result(true);
                } else {
                    rsp.Result(false);
                }
            });
        }

        void Start() {
            constexpr long Timeout = 50;
            while (work.load() && this->net.Valid()) {
                if (this->net.Wait(Timeout)) {
                    this->net.Receive();
                    this->net.Process();
                }
            }
            if (this->net.Valid()) {
                this->net.Close();
            }
        }

     private:
        KgrNetInterface net;
        const std::atomic<bool> &work;
        KgrNamedServer &server;
        std::map<int64_t, std::unique_ptr<KgrPipe>> pipes;
        int64_t nextPipeId;
        std::mutex mtx;
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
                constexpr long Timeout = 1;
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
        std::unique_lock<std::mutex> lock(this->mtx);
        if (!this->work.load()) {
            return;
        }
        this->work = false;
        this->workers.Wait([](auto count) { return count == 0; });
    }
}