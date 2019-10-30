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
#include <thread>

namespace sloked {

    KgrSlaveNetServer::KgrSlaveNetServer(std::unique_ptr<SlokedSocket> socket)
        : net(std::move(socket)), work(false), workers(0) {

        this->net.BindMethod("send", [this](const std::string &method, const KgrValue &params, auto &rsp) {
            std::unique_lock<std::mutex> lock(this->mtx);
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
            std::unique_lock<std::mutex> lock(this->mtx);
            const auto pipeId = params.AsInt();
            if (this->pipes.count(pipeId) != 0) {
                this->pipes.erase(pipeId);
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        });   
    }
    
    KgrSlaveNetServer::~KgrSlaveNetServer() {
        this->Stop();
    }

    bool KgrSlaveNetServer::IsRunning() const {
        return this->work.load();
    }

    void KgrSlaveNetServer::Start() {
        std::unique_lock<std::mutex> lock(this->mtx);
        if (this->work.load()) {
            return;
        }
        this->work = true;
        SlokedCounter<std::size_t>::Handle handle(this->workers);
        std::thread([this, handle = std::move(handle)] {
            constexpr long Timeout = 50;
            while (this->work.load() && this->net.Valid()) {
                if (this->net.Wait(Timeout)) {
                    this->net.Receive();
                    this->net.Process();
                }
            }
            if (this->net.Valid()) {
                this->net.Close();
            }
        }).detach();
        this->workers.Wait([](std::size_t count) { return count > 0; });
    }

    void KgrSlaveNetServer::Stop() {
        std::unique_lock<std::mutex> lock(this->mtx);
        if (!this->work.load()) {
            return;
        }
        this->work = false;
        this->workers.Wait([](std::size_t count) { return count == 0; });
    }

    std::unique_ptr<KgrPipe> KgrSlaveNetServer::Connect(const std::string &service) {
        if (!this->work.load()) {
            return nullptr;
        }
        auto rsp = this->net.Invoke("connect", service);
        constexpr long Timeout = 50;
        while (!rsp.WaitResponse(Timeout) && this->work.load()) {}
        if (rsp.HasResponse()) {
            const auto &res = rsp.GetResponse();
            if (res.HasResult()) {
                std::unique_lock<std::mutex> lock(this->mtx);
                int64_t pipeId = res.GetResult().AsInt();
                auto [pipe1, pipe2] = KgrLocalPipe::Make();
                pipe1->SetMessageListener([this, pipeId] {
                    std::unique_lock<std::mutex> lock(this->mtx);
                    auto &pipe = *this->pipes.at(pipeId);
                    while (!pipe.Empty()) {
                        this->net.Invoke("send", KgrDictionary {
                            { "pipe", pipeId },
                            { "data", pipe.Read() }
                        });
                    }
                });
                this->pipes.emplace(pipeId, std::move(pipe1));
                return std::move(pipe2);
            }
        }
        return nullptr;
    }
    
    KgrSlaveNetServer::Connector KgrSlaveNetServer::GetConnector(const std::string &service) {
        return [this, service] {
            return this->Connect(service);
        };
    }
    
    void KgrSlaveNetServer::Register(const std::string &, std::unique_ptr<KgrService>) {
        throw SlokedError("KgrSlaveServer: Not implemented yet");
    }

    bool KgrSlaveNetServer::Registered(const std::string &) {
        throw SlokedError("KgrSlaveServer: Not implemented yet");
    }

    void KgrSlaveNetServer::Deregister(const std::string &) {
        throw SlokedError("KgrSlaveServer: Not implemented yet");
    }
}