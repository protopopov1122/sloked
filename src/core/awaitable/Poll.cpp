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

#include "sloked/core/awaitable/Poll.h"

namespace sloked {


    SlokedDefaultIOPollThread::SlokedDefaultIOPollThread(SlokedIOPoll &poll)
        : poll(poll), work(false), nextId{0} {}

    void SlokedDefaultIOPollThread::Start(long timeout) {
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
                std::unique_lock queueLock(this->queueMtx);
                if (!this->awaitableQueue.empty()) {
                    for (auto &kv : this->awaitableQueue) {
                        this->awaitables.emplace(kv.first, std::move(kv.second));
                    }
                    this->awaitableQueue.clear();
                }
                for (auto removalId : this->removalQueue) {
                    if (this->awaitables.count(removalId) != 0) {
                        this->awaitables.erase(removalId);
                    }
                    if (this->awaitableQueue.count(removalId) != 0) {
                        this->awaitableQueue.erase(removalId);
                    }
                }
                this->removalQueue.clear();
            }
        });
    }

    void SlokedDefaultIOPollThread::Stop() {
        if (!this->work.exchange(false)) {
            return;
        }
        this->work = false;
        if (this->worker.joinable()) {
            this->worker.join();
        }
    }

    SlokedDefaultIOPollThread::Handle SlokedDefaultIOPollThread::Attach(std::unique_ptr<Awaitable> awaitable) {
        std::unique_lock lock(this->queueMtx);
        auto id = this->nextId++;
        this->awaitableQueue.emplace(id, std::move(awaitable));
        auto detacher = this->poll.Attach(this->awaitableQueue.at(id)->GetAwaitable(), [this, id] {
            if (this->awaitables.count(id) != 0) {
                this->awaitables.at(id)->Process(true);
            }
        });
        return Handle([this, id, detacher = std::move(detacher)] {
            detacher();
            std::unique_lock lock(this->queueMtx);
            this->removalQueue.push_back(id);
        });
    }
}