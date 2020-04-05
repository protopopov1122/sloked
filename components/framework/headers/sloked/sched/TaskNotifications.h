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

#ifndef SLOKED_SCHED_TASKNOTIFICATIONS_H_
#define SLOKED_SCHED_TASKNOTIFICATIONS_H_

#include "sloked/core/Closeable.h"
#include "sloked/sched/Task.h"

namespace sloked {

    class SlokedTaskNotifications : public SlokedCloseable {
     public:
        template <typename R, typename E>
        using Listener = typename TaskResult<R, E>::Listener;

        template <typename R, typename E>
        using DetachListener = typename TaskResult<R, E>::DetachListener;

        class NotificationHandle {
         public:
            enum class State { Pending, Running, Complete };

            bool Start();
            void Finish();
            void Cancel();
            void Erase();

            friend class SlokedTaskNotifications;

         private:
            NotificationHandle(SlokedTaskNotifications &, std::size_t);

            SlokedTaskNotifications &notifications;
            std::size_t id;
            State state;
            std::mutex mtx;
            std::condition_variable cv;
        };

        friend class NotificationHandle;

        template <typename R, typename E>
        DetachListener<R, E> Notify(const TaskResult<R, E> &task,
                                    Listener<R, E> listener) {
            std::unique_lock lock(this->mtx);
            if (!this->active) {
                return [] {};
            }
            auto id = this->nextId++;
            auto handle = std::shared_ptr<NotificationHandle>(
                new NotificationHandle(*this, id));
            this->handles.insert_or_assign(id, handle);
            lock.unlock();

            auto detach = task.Notify(
                [this, handle = std::move(handle),
                 listener = std::move(listener)](const auto &task) mutable {
                    if (!handle->Start()) {
                        return;
                    }
                    try {
                        listener(task);
                        handle->Finish();
                    } catch (...) {
                        handle->Finish();
                        throw;
                    }
                });
            return [detach = std::move(detach), handle = std::move(handle)] {
                detach();
                handle->Cancel();
            };
        }

        void Close() final;

     private:
        std::mutex mtx;
        std::condition_variable cv;
        bool active{true};
        std::size_t nextId{0};
        std::map<std::size_t, std::shared_ptr<NotificationHandle>> handles;
    };
}  // namespace sloked

#endif