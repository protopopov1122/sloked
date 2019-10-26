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
#include "sloked/core/StateMachine.h"
#include <thread>
#include <cstring>
#include <iostream>

namespace sloked {
    enum class ContextState {
        Initial,
        Loop,
        Destroy
    };

    class KgrMasterNetServerContext : public SlokedStateMachine<ContextState> {
     public:
        KgrMasterNetServerContext(std::unique_ptr<SlokedSocket> socket, const std::atomic<bool> &work)
            : SlokedStateMachine<ContextState>(ContextState::Initial), socket(std::move(socket)), work(work) {
            this->BindState(ContextState::Initial, &KgrMasterNetServerContext::OnInitial);
            this->BindState(ContextState::Loop, [this] {
                if (std::string_view(reinterpret_cast<const char *>(this->data.data()), this->data.size()) == "\r\n") {
                    this->Transition(ContextState::Destroy);
                }
                this->socket->Write(SlokedSpan(this->data.data(), this->data.size()));
                this->data.clear();
            });
            this->BindState(ContextState::Destroy, &KgrMasterNetServerContext::OnDestroy);
        }

        void Start() {
            constexpr long Timeout = 50;
            this->RunStep();
            while (work.load() && this->GetState() == ContextState::Loop) {
                if (this->socket->Wait(Timeout)) {
                    auto data = this->socket->Read(this->socket->Available());
                    this->data.insert(this->data.end(), data.begin(), data.end());
                    this->RunStep();
                }
            }
            this->RunTransition(ContextState::Destroy);
        }

     private:
        void OnInitial() {
            const char *MSG = "HELLO\n";
            this->socket->Write(SlokedSpan(reinterpret_cast<const uint8_t *>(MSG), strlen(MSG)));
            this->Transition(ContextState::Loop);
        }

        void OnDestroy() {
            const char *BYE = "BYE\n";
            this->socket->Write(SlokedSpan(reinterpret_cast<const uint8_t *>(BYE), strlen(BYE)));
        }

        std::unique_ptr<SlokedSocket> socket;
        const std::atomic<bool> &work;
        std::vector<uint8_t> data;
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
                        KgrMasterNetServerContext ctx(std::move(socket), this->work);
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