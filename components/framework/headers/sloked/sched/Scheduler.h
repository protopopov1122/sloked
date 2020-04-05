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
                            TaskResult<T> result)
                : task(std::move(task)), result(std::move(result)) {}

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

            TaskResult<T> Result() const {
                return this->result;
            }

         private:
            std::shared_ptr<TimerTask> task;
            TaskResult<T> result;
        };

        virtual ~SlokedScheduler() = default;

        template <typename T>
        auto At(TimePoint tp, T callable)
            -> std::shared_ptr<FutureTimerTask<decltype(std::declval<T>()())>> {
            using R = decltype(callable());
            if constexpr (std::is_void_v<R>) {
                TaskResultSupplier<R> supplier;
                auto task = this->EnqueueAt(
                    tp, [supplier, callable = std::move(callable)]() mutable {
                        supplier.Wrap([supplier, callable = std::move(
                                                     callable)]() mutable {
                            callable();
                        });
                    });
                return std::make_shared<FutureTimerTask<R>>(std::move(task),
                                                            supplier.Result());
            } else {
                TaskResultSupplier<R> supplier;
                auto task = this->EnqueueAt(
                    tp, [supplier, callable = std::move(callable)]() mutable {
                        supplier.Wrap([supplier, callable = std::move(
                                                     callable)]() mutable {
                            return callable();
                        });
                    });
                return std::make_shared<FutureTimerTask<R>>(std::move(task),
                                                            supplier.Result());
            }
        }

        template <typename T>
        auto Sleep(TimeDiff td, T callable)
            -> std::shared_ptr<FutureTimerTask<decltype(std::declval<T>()())>> {
            using R = decltype(callable());
            if constexpr (std::is_void_v<R>) {
                TaskResultSupplier<R> supplier;
                auto task = this->EnqueueSleep(
                    td, [supplier, callable = std::move(callable)]() mutable {
                        supplier.Wrap([supplier, callable = std::move(
                                                     callable)]() mutable {
                            callable();
                        });
                    });
                return std::make_shared<FutureTimerTask<R>>(std::move(task),
                                                            supplier.Result());
            } else {
                TaskResultSupplier<R> supplier;
                auto task = this->EnqueueSleep(
                    td, [supplier, callable = std::move(callable)]() mutable {
                        supplier.Wrap([supplier, callable = std::move(
                                                     callable)]() mutable {
                            return callable();
                        });
                    });
                return std::make_shared<FutureTimerTask<R>>(std::move(task),
                                                            supplier.Result());
            }
        }

        template <typename T>
        auto Interval(TimeDiff td, T callable)
            -> std::shared_ptr<FutureTimerTask<decltype(std::declval<T>()())>> {
            using R = decltype(callable());
            if constexpr (std::is_void_v<R>) {
                TaskResultSupplier<R> supplier;
                auto task = this->EnqueueInterval(
                    td, [supplier, callable = std::move(callable)]() mutable {
                        supplier.Wrap([supplier, callable = std::move(
                                                     callable)]() mutable {
                            callable();
                        });
                    });
                return std::make_shared<FutureTimerTask<R>>(std::move(task),
                                                            supplier.Result());
            } else {
                TaskResultSupplier<R> supplier;
                auto task = this->EnqueueInterval(
                    td, [supplier, callable = std::move(callable)]() mutable {
                        supplier.Wrap([supplier, callable = std::move(
                                                     callable)]() mutable {
                            return callable();
                        });
                    });
                return std::make_shared<FutureTimerTask<R>>(std::move(task),
                                                            supplier.Result());
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