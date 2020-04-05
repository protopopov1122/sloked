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

#ifndef SLOKED_SCHED_EXECUTOR_H_
#define SLOKED_SCHED_EXECUTOR_H_

#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <type_traits>

#include "sloked/core/Closeable.h"
#include "sloked/sched/Task.h"

namespace sloked {

    class SlokedExecutor {
     public:
        enum class State { Pending, Running, Finished, Canceled };

        class Task {
         public:
            virtual ~Task() = default;
            virtual State Status() const = 0;
            virtual void Wait() = 0;
            virtual void Cancel() = 0;

            virtual bool Complete() const;
        };

        template <typename T>
        class FutureTask : public Task {
         public:
            FutureTask(std::shared_ptr<Task> task, TaskResult<T> result)
                : task(std::move(task)), result(std::move(result)) {}

            State Status() const final {
                return this->task->Status();
            }

            void Wait() final {
                this->task->Wait();
            }

            void Cancel() final {
                this->task->Cancel();
            }

            TaskResult<T> Result() {
                return this->result;
            }

         private:
            std::shared_ptr<Task> task;
            TaskResult<T> result;
        };

        virtual ~SlokedExecutor() = default;

        template <typename T>
        auto Enqueue(T callable)
            -> std::shared_ptr<FutureTask<decltype(std::declval<T>()())>> {
            using R = decltype(callable());
            if constexpr (std::is_void_v<R>) {
                TaskResultSupplier<R> supplier;
                auto task = this->EnqueueCallback(
                    [supplier, callable = std::move(callable)]() mutable {
                        try {
                            callable();
                            supplier.SetResult();
                        } catch (...) {
                            supplier.SetError(std::current_exception());
                        }
                    });
                return std::make_shared<FutureTask<R>>(std::move(task),
                                                       supplier.Result());
            } else {
                TaskResultSupplier<R> supplier;
                auto task = this->EnqueueCallback(
                    [supplier, callable = std::move(callable)]() mutable {
                        try {
                            supplier.SetResult(callable());
                        } catch (...) {
                            supplier.SetError(std::current_exception());
                        }
                    });
                return std::make_shared<FutureTask<R>>(std::move(task),
                                                       supplier.Result());
            }
        }

     protected:
        virtual std::shared_ptr<Task> EnqueueCallback(
            std::function<void()>) = 0;
    };

    class SlokedRunnableTask : public SlokedExecutor::Task {
     public:
        SlokedRunnableTask(std::function<void(SlokedRunnableTask &)>,
                           std::function<void()>);

        SlokedExecutor::State Status() const final;
        void Wait() final;
        void Cancel() final;

        void Start();

     private:
        std::function<void(SlokedRunnableTask &)> cancel;
        std::function<void()> callback;
        mutable std::mutex mtx;
        std::condition_variable cv;
        SlokedExecutor::State state;
    };
}  // namespace sloked

#endif