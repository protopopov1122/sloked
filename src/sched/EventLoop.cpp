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

#include "sloked/sched/EventLoop.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedDynamicDeferredTask::SlokedDynamicDeferredTask(Callback callback)
        : callback(std::move(callback)), execute(nullptr) {}

    void SlokedDynamicDeferredTask::Wait(std::function<void()> notify) {
        if (this->callback) {
            this->execute = this->callback(notify);
            this->callback = nullptr;
        } else {
            throw SlokedError("DynamicDeferredTask: Already awaited");
        }
    }

    void SlokedDynamicDeferredTask::Run() {
        if (this->execute) {
            this->execute();
        } else {
            throw SlokedError("DynamicDeferredTask: Not awaited before execution");
        }
    }

    SlokedDefaultEventLoop::SlokedDefaultEventLoop()
        : nextId{0}, notification(nullptr) {}

    void SlokedDefaultEventLoop::Attach(std::function<void()> task) {
        std::unique_lock lock(this->mtx);
        this->pending.push_back(std::move(task));
        if (this->notification) {
            this->notification();
        }
    }

    void SlokedDefaultEventLoop::Attach(std::unique_ptr<SlokedDeferredTask> task) {
        std::unique_lock lock(this->mtx);
        int64_t taskId = this->nextId++;
        this->deferred.emplace(taskId, std::move(task));
        auto &taskRef = *this->deferred.at(taskId);
        lock.unlock();
        taskRef.Wait([this, taskId] {
            std::unique_lock lock(this->mtx);
            std::shared_ptr<SlokedDeferredTask> task = std::move(this->deferred[taskId]);
            this->deferred.erase(taskId);
            this->pending.push_back([task = std::move(task)] {
                task->Run();
            });
            if (this->notification) {
                this->notification();
            }
        });
    }

    bool SlokedDefaultEventLoop::HasPending() const {
        std::unique_lock lock(this->mtx);
        return !this->pending.empty();
    }

    void SlokedDefaultEventLoop::Run() {
        std::unique_lock lock(this->mtx);
        auto pending = std::move(this->pending);
        this->pending.clear();
        lock.unlock();
        for (const auto &task : pending) {
            task();
        }
    }

    void SlokedDefaultEventLoop::Notify(std::function<void()> callback) {
        std::unique_lock lock(this->mtx);
        this->notification = std::move(callback);
    }
}