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

#include "sloked/sched/TaskNotifications.h"

namespace sloked {

    SlokedTaskNotifications::NotificationHandle::NotificationHandle(
        SlokedTaskNotifications &notifications, std::size_t id)
        : notifications(notifications), id(id), state{State::Pending} {}

    bool SlokedTaskNotifications::NotificationHandle::Start() {
        std::unique_lock lock(this->mtx);
        if (this->state == State::Pending) {
            this->state = State::Running;
            return true;
        } else {
            return false;
        }
    }

    void SlokedTaskNotifications::NotificationHandle::Finish() {
        std::unique_lock lock(this->mtx);
        this->state = State::Complete;
        this->Erase();
    }

    void SlokedTaskNotifications::NotificationHandle::Cancel() {
        std::unique_lock lock(this->mtx);
        switch (this->state) {
            case State::Pending:
                this->state = State::Complete;
                this->Erase();
                break;

            case State::Running:
                this->cv.wait(lock,
                              [this] { return this->state == State::Running; });
                break;

            case State::Complete:
                break;
        }
    }

    void SlokedTaskNotifications::NotificationHandle::Erase() {
        std::unique_lock lock(this->notifications.mtx);
        this->notifications.handles.erase(this->id);
        this->notifications.cv.notify_all();
    }

    void SlokedTaskNotifications::Close() {
        std::unique_lock lock(this->mtx);
        this->active = false;
        std::vector<std::shared_ptr<NotificationHandle>> allHandles;
        for (auto &kv : handles) {
            allHandles.emplace_back(std::move(kv.second));
        }
        lock.unlock();
        for (auto &handle : allHandles) {
            handle->Cancel();
        }
        lock.lock();
        this->cv.wait(lock, [&] { return this->handles.empty(); });
    }
}  // namespace sloked