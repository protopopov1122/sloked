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

#include "sloked/net/Poll.h"

namespace sloked {


    SlokedDefaultSocketPollThread::SlokedDefaultSocketPollThread(SlokedSocketPoll &poll)
        : poll(poll), work(false), nextId{0} {}

    void SlokedDefaultSocketPollThread::Start(long timeout) {
        if (this->work.exchange(true)) {
            return;
        }
        this->work = true;
        this->worker = std::thread([this, timeout] {
            while (this->work.load()) {
                for (const auto &kv : this->awaitables) {
                    kv.second->Process(false);
                }
                this->poll.Await(timeout);
            }
        });
    }

    void SlokedDefaultSocketPollThread::Stop() {
        if (!this->work.exchange(false)) {
            return;
        }
        this->work = false;
        if (this->worker.joinable()) {
            this->worker.join();
        }
    }

    SlokedDefaultSocketPollThread::Handle SlokedDefaultSocketPollThread::Attach(std::unique_ptr<Awaitable> awaitable) {
        std::unique_lock lock(this->mtx);
        auto id = this->nextId++;
        this->awaitables.emplace(id, std::move(awaitable));
        auto detacher = this->poll.Attach(this->awaitables.at(id)->GetAwaitable(), [this, id] {
            std::unique_lock lock(this->mtx);
            if (this->awaitables.count(id) != 0) {
                this->awaitables.at(id)->Process(true);
            }
        });
        return Handle([this, id, detacher = std::move(detacher)] {
            std::unique_lock lock(this->mtx);
            detacher();
            if (this->awaitables.count(id) != 0) {
                this->awaitables.erase(id);
            }
        });
    }
}