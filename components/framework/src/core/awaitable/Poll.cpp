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

#include "sloked/core/awaitable/Poll.h"

namespace sloked {

    SlokedDefaultIOPollThread::SlokedDefaultIOPollThread(
        SlokedIOPoll &poll, SlokedExecutor &executor)
        : poll(poll), executor(executor),
          work(false), nextId{0}, asyncTasks{0} {}

    SlokedDefaultIOPollThread::~SlokedDefaultIOPollThread() {
        this->Close();
    }

    void SlokedDefaultIOPollThread::Start(
        std::chrono::system_clock::duration timeout) {
        if (this->work.exchange(true)) {
            return;
        }
        this->work = true;
        this->worker = std::thread([this, timeout] {
            while (this->work.load()) {
                this->poll.Await(timeout);
                std::unique_lock lock(this->mtx);
                std::vector<std::shared_ptr<SlokedExecutor::Task>> tasks;
                for (const auto &kv : this->awaitables) {
                    tasks.emplace_back(
                        this->executor.Enqueue([awaitable = kv.second] {
                            awaitable->Process(false);
                        }));
                }
                lock.unlock();
                for (auto task : tasks) {
                    task->Wait();
                }
            }
        });
    }

    void SlokedDefaultIOPollThread::Close() {
        if (this->work.exchange(false) && this->worker.joinable()) {
            this->worker.join();
        }
    }

    SlokedDefaultIOPollThread::Handle SlokedDefaultIOPollThread::Attach(
        std::unique_ptr<Awaitable> awaitable) {
        std::unique_lock lock(this->mtx);
        auto id = this->nextId++;
        this->awaitables.insert_or_assign(id, std::move(awaitable));
        auto detacher = this->poll.Attach(
            this->awaitables.at(id)->GetAwaitable(), [this, id] {
                std::unique_lock lock(this->mtx);
                if (this->awaitables.count(id)) {
                    auto awaitable = this->awaitables.at(id);
                    lock.unlock();
                    this->executor
                        .Enqueue([this, awaitable = std::move(awaitable)] {
                            awaitable->Process(true);
                        })
                        ->Wait();
                }
            });
        return Handle([this, id, detacher = std::move(detacher)] {
            detacher();
            std::unique_lock lock(this->mtx);
            if (this->awaitables.count(id)) {
                this->awaitables.erase(id);
            }
        });
    }
}  // namespace sloked