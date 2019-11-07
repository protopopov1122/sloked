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

#include "sloked/core/EventLoop.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedDynamicAsyncTask::SlokedDynamicAsyncTask(Callback callback)
        : callback(std::move(callback)), execute(nullptr) {}

    void SlokedDynamicAsyncTask::Wait(std::function<void()> notify) {
        this->execute = this->callback(notify);
    }

    bool SlokedDynamicAsyncTask::Run() {
        if (this->execute) {
            return this->execute();
        } else {
            throw SlokedError("DynamicAsyncTask: Not awaited before execution");
        }
    }

    void SlokedDefaultEventLoop::Attach(std::unique_ptr<SlokedAsyncTask> task) {
        std::unique_lock lock(this->mtx);
        int64_t taskId = this->nextId++;
        this->deferred.emplace(taskId, std::move(task));
        this->deferred.at(taskId)->Wait([this, taskId] {
            std::unique_lock lock(this->mtx);
            auto task = std::move(this->deferred[taskId]);
            this->deferred.erase(taskId);
            this->pending.push(std::move(task));
            if (this->callback) {
                this->callback();
            }
        });
    }

    bool SlokedDefaultEventLoop::HasPending() const {
        std::unique_lock lock(this->mtx);
        return !this->pending.empty();
    }

    void SlokedDefaultEventLoop::Run() {
        std::unique_lock lock(this->mtx);
        while (!this->pending.empty()) {
            auto task = std::move(this->pending.front());
            this->pending.pop();
            lock.unlock();
            bool result = task->Run();
            lock.lock();
            if (result) {
                this->Attach(std::move(task));
            }
        }
    }

    void SlokedDefaultEventLoop::SetListener(std::function<void()> callback) {
        std::unique_lock lock(this->mtx);
        this->callback = std::move(callback);
    }
}