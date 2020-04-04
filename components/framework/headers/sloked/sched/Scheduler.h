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

#ifndef SLOKED_SCHED_SCHEDULER_H_
#define SLOKED_SCHED_SCHEDULER_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <optional>
#include <queue>
#include <type_traits>
#include <utility>

#include "sloked/core/Closeable.h"
#include "sloked/core/Counter.h"
#include "sloked/sched/Executor.h"

namespace sloked {

    class SlokedScheduler {
     public:
        using Clock = std::chrono::system_clock;
        using TimePoint = std::chrono::time_point<Clock, Clock::duration>;
        using TimeDiff = Clock::duration;
        using Callback = std::function<void()>;

        class TimerTask : public SlokedExecutor::Task {
         public:
            virtual ~TimerTask() = default;
            virtual bool Pending() const;
            virtual TimePoint GetTime() const = 0;
            virtual bool IsRecurring() const = 0;
            virtual std::optional<TimeDiff> GetInterval() const = 0;
        };

        template <typename T>
        class FutureTimerTask : public TimerTask {
         public:
            FutureTimerTask(std::shared_ptr<TimerTask> task,
                            std::future<T> future)
                : task(std::move(task)), future(std::move(future)) {}

            SlokedExecutor::State Status() const final {
                return this->task->Status();
            }

            void Wait() final {
                this->task->Wait();
            }

            void Cancel() final {
                this->task->Cancel();
            }

            TimePoint GetTime() const final {
                return this->task->GetTime();
            }

            bool IsRecurring() const final {
                return this->task->IsRecurring();
            }

            std::optional<TimeDiff> GetInterval() const final {
                return this->task->GetInterval();
            }

            std::future<T> &GetFuture() const {
                return this->future;
            }

         private:
            std::shared_ptr<TimerTask> task;
            std::future<T> future;
        };

        virtual ~SlokedScheduler() = default;

        template <typename T>
        auto At(TimePoint tp, T callable)
            -> std::shared_ptr<FutureTimerTask<decltype(std::declval<T>()())>> {
            using R = decltype(callable());
            if constexpr (std::is_void_v<R>) {
                auto promise = std::make_shared<std::promise<void>>();
                auto task = this->EnqueueAt(
                    tp, [promise, callable = std::move(callable)]() mutable {
                        try {
                            callable();
                            promise->set_value();
                        } catch (...) {
                            promise->set_exception(std::current_exception());
                        }
                    });
                return std::make_shared<FutureTimerTask<R>>(
                    std::move(task), promise->get_future());
            } else {
                auto promise = std::make_shared<std::promise<R>>();
                auto task = this->EnqueueAt(
                    tp, [promise, callable = std::move(callable)]() mutable {
                        try {
                            promise->set_value(callable());
                        } catch (...) {
                            promise->set_exception(std::current_exception());
                        }
                    });
                return std::make_shared<FutureTimerTask<R>>(
                    std::move(task), promise->get_future());
            }
        }

        template <typename T>
        auto Sleep(TimeDiff td, T callable)
            -> std::shared_ptr<FutureTimerTask<decltype(std::declval<T>()())>> {
            using R = decltype(callable());
            if constexpr (std::is_void_v<R>) {
                auto promise = std::make_shared<std::promise<void>>();
                auto task = this->EnqueueSleep(
                    td, [promise, callable = std::move(callable)]() mutable {
                        try {
                            callable();
                            promise->set_value();
                        } catch (...) {
                            promise->set_exception(std::current_exception());
                        }
                    });
                return std::make_shared<FutureTimerTask<R>>(
                    std::move(task), promise->get_future());
            } else {
                auto promise = std::make_shared<std::promise<R>>();
                auto task = this->EnqueueSleep(
                    td, [promise, callable = std::move(callable)]() mutable {
                        try {
                            promise->set_value(callable());
                        } catch (...) {
                            promise->set_exception(std::current_exception());
                        }
                    });
                return std::make_shared<FutureTimerTask<R>>(
                    std::move(task), promise->get_future());
            }
        }

        template <typename T>
        auto Interval(TimeDiff td, T callable)
            -> std::shared_ptr<FutureTimerTask<decltype(std::declval<T>()())>> {
            using R = decltype(callable());
            if constexpr (std::is_void_v<R>) {
                auto promise = std::make_shared<std::promise<void>>();
                auto task = this->EnqueueInterval(
                    td, [promise, callable = std::move(callable)]() mutable {
                        try {
                            callable();
                            promise->set_value();
                        } catch (...) {
                            promise->set_exception(std::current_exception());
                        }
                    });
                return std::make_shared<FutureTimerTask<R>>(
                    std::move(task), promise->get_future());
            } else {
                auto promise = std::make_shared<std::promise<R>>();
                auto task = this->EnqueueInterval(
                    td, [promise, callable = std::move(callable)]() mutable {
                        try {
                            promise->set_value(callable());
                        } catch (...) {
                            promise->set_exception(std::current_exception());
                        }
                    });
                return std::make_shared<FutureTimerTask<R>>(
                    std::move(task), promise->get_future());
            }
        }

     protected:
        virtual std::shared_ptr<TimerTask> EnqueueAt(TimePoint, Callback) = 0;
        virtual std::shared_ptr<TimerTask> EnqueueSleep(TimeDiff, Callback) = 0;
        virtual std::shared_ptr<TimerTask> EnqueueInterval(TimeDiff,
                                                           Callback) = 0;
    };
}  // namespace sloked

#endif