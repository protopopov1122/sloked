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

#include "sloked/sched/ScopedExecutor.h"

#include <algorithm>

namespace sloked {

    SlokedScopedExecutor::ScopedTask::ScopedTask(SlokedScopedExecutor &executor,
                                                 std::size_t id,
                                                 std::shared_ptr<Task> task)
        : executor(executor), id(id), task(std::move(task)) {}

    SlokedExecutor::State SlokedScopedExecutor::ScopedTask::Status() const {
        return this->task->Status();
    }
    void SlokedScopedExecutor::ScopedTask::Wait() {
        this->task->Wait();
    }

    void SlokedScopedExecutor::ScopedTask::Cancel() {
        this->task->Cancel();
        if (this->task->Status() == SlokedExecutor::State::Canceled) {
            std::unique_lock lock(this->executor.mtx);
            if (this->executor.tasks.count(id)) {
                this->executor.tasks.erase(id);
            }
        }
    }

    SlokedScopedExecutor::SlokedScopedExecutor(SlokedExecutor &executor)
        : executor(executor), nextId{0} {}

    void SlokedScopedExecutor::Close() {
        std::unique_lock lock(this->mtx);
        for (auto it = this->tasks.begin(); it != this->tasks.end();) {
            auto task = (it++)->second;
            lock.unlock();
            task->Cancel();
            lock.lock();
        }
    }

    std::shared_ptr<SlokedExecutor::Task> SlokedScopedExecutor::EnqueueCallback(
        std::function<void()> callback) {
        std::unique_lock lock(this->mtx);
        auto id = this->nextId++;
        auto rawTask = this->executor.Enqueue(
            [this, id, callback = std::move(callback)]() mutable {
                callback();
                std::unique_lock lock(this->mtx);
                if (this->tasks.count(id)) {
                    this->tasks.erase(id);
                }
            });
        return std::make_shared<ScopedTask>(*this, id, std::move(rawTask));
    }
}  // namespace sloked