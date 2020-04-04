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
#include "sloked/sched/Executor.h"

#include <algorithm>

namespace sloked {

    bool SlokedExecutor::Task::Complete() const {
        auto state = this->Status();
        return state == State::Finished || state == State::Canceled;
    }

    SlokedRunnableTask::SlokedRunnableTask(
        std::function<void(SlokedRunnableTask &)> cancel,
        std::function<void()> callback)
        : cancel(std::move(cancel)),
          callback(std::move(callback)), state{SlokedExecutor::State::Pending} {
    }

    SlokedExecutor::State SlokedRunnableTask::Status() const {
        std::unique_lock lock(this->mtx);
        return this->state;
    }

    void SlokedRunnableTask::Wait() {
        std::unique_lock lock(this->mtx);
        this->cv.wait(lock, [this] {
            return state == SlokedExecutor::State::Finished ||
                   state == SlokedExecutor::State::Canceled;
        });
    }

    void SlokedRunnableTask::Cancel() {
        std::unique_lock lock(this->mtx);
        if (this->state == SlokedExecutor::State::Pending) {
            this->state = SlokedExecutor::State::Canceled;
            this->cancel(*this);
            this->cv.notify_all();
        } else if (this->state == SlokedExecutor::State::Running) {
            this->cv.wait(lock, [this] {
                return this->state == SlokedExecutor::State::Finished;
            });
        }
    }

    void SlokedRunnableTask::Start() {
        std::unique_lock lock(this->mtx);
        this->state = SlokedExecutor::State::Running;
        lock.unlock();
        this->callback();
        lock.lock();
        this->state = SlokedExecutor::State::Finished;
        this->cv.notify_all();
    }
}  // namespace sloked