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
#include <memory>
#include <mutex>
#include <type_traits>
#include <vector>

#include "sloked/core/Error.h"
#include "sloked/core/Meta.h"
#include "sloked/sched/Lifetime.h"

namespace sloked {

    template <template <typename, typename, typename = void> typename, typename,
              typename>
    class TaskResultSupplierBase;

    template <typename, typename = std::exception_ptr, typename = void>
    class TaskResultSupplier;

    template <typename, typename = std::exception_ptr, typename = void>
    class TaskResult;

    enum class TaskResultStatus { Pending, Ready, Error, Cancelled };

    template <typename T>
    struct TaskResultUnwrapWaitReturns {
        using Type = T &;
    };

    template <>
    struct TaskResultUnwrapWaitReturns<void> {
        using Type = void;
    };

    template <template <typename, typename, typename = void> class C,
              typename R, typename E>
    class TaskResultBase {
        using UnwrapWaitType = typename TaskResultUnwrapWaitReturns<R>::Type;

     public:
        using Listener = std::function<void(const TaskResult<R, E, void> &)>;
        using DetachListener = std::function<void()>;
        using Status = TaskResultStatus;
        using Supplier = TaskResultSupplier<R, E, void>;
        using Result = R;
        using Error = E;

        Status State() const {
            std::unique_lock lock(this->impl->mtx);
            return this->impl->state;
        }

        E &GetError() const {
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

        template <typename Tp>
        Status WaitUntil(Tp tpoint) const {
            std::unique_lock lock(this->impl->mtx);
            this->impl->cv.wait_until(lock, tpoint, [this] {
                return this->impl->state != Status::Pending;
            });
            return this->impl->state;
        }

        DetachListener Notify(Listener listener,
                              std::weak_ptr<SlokedLifetime> lifetime =
                                  SlokedLifetime::Global) const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Pending) {
                auto id = this->impl->nextListenerId++;
                this->impl->listeners.insert_or_assign(
                    id,
                    [lifetime_weak = lifetime, listener = std::move(listener)](
                        const auto &result) mutable {
                        if (auto lifetime = lifetime_weak.lock()) {
                            if (auto token = lifetime->Acquire()) {
                                listener(result);
                            }
                        }
                    });
                std::weak_ptr<Impl> impl_weak = this->impl;
                return [id, impl_weak] {
                    if (auto impl = impl_weak.lock()) {
                        std::unique_lock lock(impl->mtx);
                        impl->listeners.erase(id);
                    }
                };
            } else {
                lock.unlock();
                if (auto ltime = lifetime.lock()) {
                    if (auto token = ltime->Acquire()) {
                        listener(*static_cast<const C<R, E> *>(this));
                    }
                }
                return [] {};
            }
        }

        auto UnwrapWait() const -> UnwrapWaitType {
            this->Wait();
            if constexpr (std::is_void_v<R>) {
                static_cast<const C<R, E> *>(this)->Unwrap();
            } else {
                return static_cast<const C<R, E> *>(this)->Unwrap();
            }
        }

        template <typename D>
        auto UnwrapWaitFor(D duration) const -> UnwrapWaitType {
            this->WaitFor<D>(duration);
            if constexpr (std::is_void_v<R>) {
                static_cast<const C<R, E> *>(this)->Unwrap();
            } else {
                return static_cast<const C<R, E> *>(this)->Unwrap();
            }
        }

        template <typename Tp>
        auto UnwrapWaitUntil(Tp tpoint) const -> UnwrapWaitType {
            this->WaitUntil<Tp>(tpoint);
            if constexpr (std::is_void_v<R>) {
                static_cast<const C<R, E> *>(this)->Unwrap();
            } else {
                return static_cast<const C<R, E> *>(this)->Unwrap();
            }
        }

     protected:
        struct NoValue {};

        template <typename... T>
        struct ImplBase {
            Status state{Status::Pending};
            UniqueVariant_t<NoValue, T...> result{NoValue{}};
            std::size_t nextListenerId{0};
            std::map<std::size_t, Listener> listeners;
            std::mutex mtx;
            std::condition_variable cv;
        };

        using Impl =
            std::conditional_t<std::is_void_v<R>, ImplBase<E>, ImplBase<R, E>>;

        TaskResultBase(std::shared_ptr<Impl> impl) : impl(std::move(impl)) {}

        std::shared_ptr<Impl> impl;
    };

    template <typename R, typename E>
    class TaskResult<R, E, std::enable_if_t<!std::is_void_v<R>>>
        : public TaskResultBase<TaskResult, R, E> {
        using Parent = TaskResultBase<TaskResult, R, E>;

     public:
        using Listener = typename Parent::Listener;
        using DetachListener = typename Parent::DetachListener;
        using Status = typename Parent::Status;
        friend class TaskResultSupplierBase<TaskResultSupplier, R, E>;

        R &GetResult() const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Ready) {
                return std::get<R>(this->impl->result);
            } else {
                throw SlokedError("TaskResult: Reault is not available");
            }
        }

        R &Unwrap() const {
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

        static TaskResult<R, E> Resolve(R &&result) {
            auto impl = std::make_shared<typename Parent::Impl>();
            impl->state = Status::Ready;
            impl->result = std::forward<R>(result);
            return TaskResult<R, E>(std::move(impl));
        }

        static TaskResult<R, E> Reject(E &&error) {
            auto impl = std::make_shared<typename Parent::Impl>();
            impl->state = Status::Error;
            impl->result = std::forward<E>(error);
            return TaskResult<R, E>(std::move(impl));
        }

        static TaskResult<R, E> Cancel() {
            auto impl = std::make_shared<typename Parent::Impl>();
            impl->state = Status::Cancelled;
            return TaskResult<R, E>(std::move(impl));
        }

     private:
        using Parent::Parent;
    };

    template <typename E>
    class TaskResult<void, E, void>
        : public TaskResultBase<TaskResult, void, E> {
        using Parent = TaskResultBase<TaskResult, void, E>;

     public:
        using Listener = typename Parent::Listener;
        using DetachListener = typename Parent::DetachListener;
        using Status = typename Parent::Status;
        friend class TaskResultSupplierBase<TaskResultSupplier, void, E>;

        void Unwrap() const {
            std::unique_lock lock(this->impl->mtx);
            if (this->impl->state == Status::Error) {
                if constexpr (std::is_same_v<E, std::exception_ptr>) {
                    std::rethrow_exception(std::get<E>(this->impl->result));
                } else {
                    throw std::get<E>(this->impl->result);
                }
            } else if (this->impl->state == Status::Cancelled) {
                throw SlokedError("TaskResult: Task is cancelled");
            } else if (this->impl->state == Status::Pending) {
                throw SlokedError("TaskResult: Result is not available");
            }
        }

        static TaskResult<void, E> Resolve() {
            auto impl = std::make_shared<typename Parent::Impl>();
            impl->state = Status::Ready;
            return TaskResult<void, E>(std::move(impl));
        }

        static TaskResult<void, E> Reject(E &&error) {
            auto impl = std::make_shared<typename Parent::Impl>();
            impl->state = Status::Error;
            impl->result = std::forward<E>(error);
            return TaskResult<void, E>(std::move(impl));
        }

        static TaskResult<void, E> Cancel() {
            auto impl = std::make_shared<typename Parent::Impl>();
            impl->state = Status::Cancelled;
            return TaskResult<void, E>(std::move(impl));
        }

     private:
        using Parent::Parent;
    };

    template <template <typename, typename, typename = void> class C,
              typename R, typename E>
    class TaskResultSupplierBase {
        using Status = TaskResultStatus;
        using Impl = typename TaskResult<R, E>::Impl;

     public:
        TaskResultSupplierBase() : impl(std::make_shared<Impl>()) {}

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

        template <typename T>
        void Catch(T callable) const {
            if constexpr (std::is_same_v<std::exception_ptr, E>) {
                try {
                    callable();
                } catch (...) { this->SetError(std::current_exception()); }
            } else {
                try {
                    callable();
                } catch (const E &error) { this->SetError(error); }
            }
        }

        template <typename T>
        void Wrap(T callable) const {
            this->Catch([&] {
                if constexpr (std::is_void_v<R>) {
                    callable();
                    static_cast<const C<R, E> *>(this)->SetResult();
                } else {
                    static_cast<const C<R, E> *>(this)->SetResult(callable());
                }
            });
        }

     protected:
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

        std::shared_ptr<Impl> impl;
    };

    template <typename R, typename E>
    class TaskResultSupplier<R, E, std::enable_if_t<!std::is_void_v<R>>>
        : public TaskResultSupplierBase<TaskResultSupplier, R, E> {
        using Status = TaskResultStatus;

     public:
        using TaskResultSupplierBase<TaskResultSupplier, R,
                                     E>::TaskResultSupplierBase;

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
    };

    template <typename E>
    class TaskResultSupplier<void, E, void>
        : public TaskResultSupplierBase<TaskResultSupplier, void, E> {
        using Status = TaskResultStatus;

     public:
        using TaskResultSupplierBase<TaskResultSupplier, void,
                                     E>::TaskResultSupplierBase;

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
    };
}  // namespace sloked

#endif
