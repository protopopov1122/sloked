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

#ifndef SLOKED_SCHED_TASK_H_
#define SLOKED_SCHED_TASK_H_

#include <chrono>
#include <condition_variable>
#include <exception>
#include <future>
#include <map>
#include <mutex>
#include <type_traits>
#include <variant>

#include "sloked/core/Error.h"

namespace sloked {

    template <typename R, typename E = std::exception_ptr, typename = void>
    class TaskResultSupplier;

    template <typename R, typename E = std::exception_ptr, typename = void>
    class TaskResult;

    template <typename R, typename E>
    class TaskResult<R, E, std::enable_if_t<!std::is_void_v<R>>> {
     public:
        using Listener = std::function<void(const TaskResult<R, E> &)>;
        using DetachListener = std::function<void()>;

        enum class Status { Pending, Ready, Error, Cancelled };

        friend class TaskResultSupplier<R, E>;

        TaskResult(const TaskResult &) = default;
        TaskResult(TaskResult &&) = default;
        ~TaskResult() = default;

        TaskResult &operator=(const TaskResult &) = default;
        TaskResult &operator=(TaskResult &&) = default;

        Status State() const {
            std::unique_lock lock(this->impl->mtx);
            return this->impl->state;
        }

        const R &GetResult() const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Ready) {
                return std::get<R>(this->impl->result);
            } else {
                throw SlokedError("TaskResult: Reault is not available");
            }
        }

        const E &GetError() const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Error) {
                return std::get<E>(this->impl->result);
            } else {
                throw SlokedError("TaskResult: Error is not available");
            }
        }

        const R &Get() const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Ready) {
                return std::get<R>(this->impl->result);
            } else if (this->impl->state == Status::Error) {
                if constexpr (std::is_same_v<E, std::exception_ptr>) {
                    std::rethrow_exception(std::get<E>(this->impl->result));
                } else {
                    throw std::get<E>(this->impl->result);
                }
            } else if (this->impl->state == Status::Cancelled) {
                throw SlokedError("TaskResult: Task is cancelled");
            } else {
                throw SlokedError("TaskResult: Result is not available");
            }
        }

        Status Wait() const {
            std::unique_lock lock(this->impl->mtx);
            this->impl->cv.wait(
                lock, [this] { return this->impl->state != Status::Pending; });
            return this->impl->state;
        }

        template <typename D>
        Status WaitFor(D duration) const {
            std::unique_lock lock(this->impl->mtx);
            this->impl->cv.wait_for(lock, duration, [this] {
                return this->impl->state != Status::Pending;
            });
            return this->impl->state;
        }

        template <typename T>
        Status WaitUntil(T tpoint) const {
            std::unique_lock lock(this->impl->mtx);
            this->impl->cv.wait_until(lock, tpoint, [this] {
                return this->impl->state != Status::Pending;
            });
            return this->impl->state;
        }

        DetachListener Notify(Listener listener) {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Pending) {
                auto id = this->impl->nextListenerId++;
                this->impl->listeners.insert_or_assign(id, std::move(listener));
                return [id, impl_weak = std::weak_ptr(this->impl)] {
                    if (auto impl = impl_weak.lock()) {
                        std::unique_lock lock(impl->mtx);
                        impl->listeners.erase(id);
                    }
                };
            } else {
                lock.unlock();
                listener(*this);
                return [] {};
            }
        }

     private:
        struct NoValue {};
        struct Impl {
            Status state{Status::Pending};
            std::variant<NoValue, R, E> result{NoValue{}};
            std::size_t nextListenerId{0};
            std::map<std::size_t, Listener> listeners;
            std::mutex mtx;
            std::condition_variable cv;
        };

        TaskResult(std::shared_ptr<Impl> impl) : impl(std::move(impl)) {}

        std::shared_ptr<Impl> impl;
    };

    template <typename E>
    class TaskResult<void, E, void> {
     public:
        using Listener = std::function<void(const TaskResult<void, E> &)>;
        using DetachListener = std::function<void()>;

        enum class Status { Pending, Ready, Error, Cancelled };

        friend class TaskResultSupplier<void, E>;

        TaskResult(const TaskResult &) = default;
        TaskResult(TaskResult &&) = default;
        ~TaskResult() = default;

        TaskResult &operator=(const TaskResult &) = default;
        TaskResult &operator=(TaskResult &&) = default;

        Status State() const {
            std::unique_lock lock(this->impl->mtx);
            return this->impl->state;
        }

        const E &GetError() const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Error) {
                return std::get<E>(this->impl->result);
            } else {
                throw SlokedError("TaskResult: Error is not available");
            }
        }

        Status Wait() const {
            std::unique_lock lock(this->impl->mtx);
            this->impl->cv.wait(
                lock, [this] { return this->impl->state != Status::Pending; });
            return this->impl->state;
        }

        template <typename D>
        Status WaitFor(D duration) const {
            std::unique_lock lock(this->impl->mtx);
            this->impl->cv.wait_for(lock, duration, [this] {
                return this->impl->state != Status::Pending;
            });
            return this->impl->state;
        }

        template <typename T>
        Status WaitUntil(T tpoint) const {
            std::unique_lock lock(this->impl->mtx);
            this->impl->cv.wait_until(lock, tpoint, [this] {
                return this->impl->state != Status::Pending;
            });
            return this->impl->state;
        }

        DetachListener Notify(Listener listener) {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Pending) {
                auto id = this->impl->nextListenerId++;
                this->impl->listeners.insert_or_assign(id, std::move(listener));
                return [id, impl_weak = std::weak_ptr(this->impl)] {
                    if (auto impl = impl_weak.lock()) {
                        std::unique_lock lock(impl->mtx);
                        impl->listeners.erase(id);
                    }
                };
            } else {
                lock.unlock();
                listener(*this);
                return [] {};
            }
        }

     private:
        struct NoValue {};
        struct Impl {
            Status state{Status::Pending};
            std::variant<NoValue, E> result{NoValue{}};
            std::size_t nextListenerId{0};
            std::map<std::size_t, Listener> listeners;
            std::mutex mtx;
            std::condition_variable cv;
        };

        TaskResult(std::shared_ptr<Impl> impl) : impl(std::move(impl)) {}

        std::shared_ptr<Impl> impl;
    };

    template <typename R, typename E>
    class TaskResultSupplier<R, E, std::enable_if_t<!std::is_void_v<R>>> {
        using Status = typename TaskResult<R, E>::Status;

     public:
        TaskResultSupplier()
            : impl(std::make_shared<typename TaskResult<R, E>::Impl>()) {}

        void SetResult(const R &result) const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Pending) {
                this->impl->state = Status::Ready;
                this->impl->result = result;
                this->TriggerListeners(std::move(lock));
            } else {
                throw SlokedError(
                    "TaskResultSupplier: Can't set result of non-pending task");
            }
        }

        void SetResult(R &&result) const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Pending) {
                this->impl->state = Status::Ready;
                this->impl->result = std::forward<R>(result);
                this->TriggerListeners(std::move(lock));
            } else {
                throw SlokedError(
                    "TaskResultSupplier: Can't set result of non-pending task");
            }
        }

        void SetError(const E &error) const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Pending) {
                this->impl->state = Status::Error;
                this->impl->result = error;
                this->TriggerListeners(std::move(lock));
            } else {
                throw SlokedError(
                    "TaskResultSupplier: Can't set error of non-pending task");
            }
        }

        void SetError(E &&error) const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Pending) {
                this->impl->state = Status::Error;
                this->impl->result = std::forward<E>(error);
                this->TriggerListeners(std::move(lock));
            } else {
                throw SlokedError(
                    "TaskResultSupplier: Can't set error of non-pending task");
            }
        }

        void Cancel() const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Pending) {
                this->impl->state = Status::Cancelled;
                this->TriggerListeners(std::move(lock));
            } else {
                throw SlokedError(
                    "TaskResultSupplier: Can't cancel non-pending task");
            }
        }

        TaskResult<R, E> Result() const {
            return TaskResult<R, E>(this->impl);
        }

     private:
        void TriggerListeners(std::unique_lock<std::mutex> lock) const {
            this->impl->cv.notify_all();
            std::vector<typename TaskResult<R, E>::Listener> callbacks;
            for (auto listener : this->impl->listeners) {
                callbacks.emplace_back(std::move(listener.second));
            }
            this->impl->listeners.clear();
            lock.unlock();
            for (auto listener : callbacks) {
                listener(TaskResult<R, E>(this->impl));
            }
        }

        std::shared_ptr<typename TaskResult<R, E>::Impl> impl;
    };

    template <typename E>
    class TaskResultSupplier<void, E, void> {
        using Status = typename TaskResult<void, E>::Status;

     public:
        TaskResultSupplier()
            : impl(std::make_shared<typename TaskResult<void, E>::Impl>()) {}

        void SetResult() const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Pending) {
                this->impl->state = Status::Ready;
                this->TriggerListeners(std::move(lock));
            } else {
                throw SlokedError(
                    "TaskResultSupplier: Can't set result of non-pending task");
            }
        }

        void SetError(const E &error) const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Pending) {
                this->impl->state = Status::Error;
                this->impl->result = error;
                this->TriggerListeners(std::move(lock));
            } else {
                throw SlokedError(
                    "TaskResultSupplier: Can't set error of non-pending task");
            }
        }

        void SetError(E &&error) const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Pending) {
                this->impl->state = Status::Error;
                this->impl->result = std::forward<E>(error);
                this->TriggerListeners(std::move(lock));
            } else {
                throw SlokedError(
                    "TaskResultSupplier: Can't set error of non-pending task");
            }
        }

        void Cancel() const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Pending) {
                this->impl->state = Status::Cancelled;
                this->TriggerListeners(std::move(lock));
            } else {
                throw SlokedError(
                    "TaskResultSupplier: Can't cancel non-pending task");
            }
        }

        TaskResult<void, E> Result() const {
            return TaskResult<void, E>(this->impl);
        }

     private:
        void TriggerListeners(std::unique_lock<std::mutex> lock) const {
            this->impl->cv.notify_all();
            std::vector<typename TaskResult<void, E>::Listener> callbacks;
            for (auto listener : this->impl->listeners) {
                callbacks.emplace_back(std::move(listener.second));
            }
            this->impl->listeners.clear();
            lock.unlock();
            for (auto listener : callbacks) {
                listener(TaskResult<void, E>(this->impl));
            }
        }

        std::shared_ptr<typename TaskResult<void, E>::Impl> impl;
    };
}  // namespace sloked

#endif