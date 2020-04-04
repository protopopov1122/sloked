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
            FutureTask(std::shared_ptr<Task> task, std::future<T> future)
                : task(std::move(task)), future(std::move(future)) {}

            State Status() const final {
                return this->task->Status();
            }

            void Wait() final {
                this->task->Wait();
            }

            void Cancel() final {
                this->task->Cancel();
            }

            std::future<T> &Result() {
                return this->future;
            }

         private:
            std::shared_ptr<Task> task;
            std::future<T> future;
        };

        virtual ~SlokedExecutor() = default;

        template <typename T>
        auto Enqueue(T callable)
            -> std::shared_ptr<FutureTask<decltype(std::declval<T>()())>> {
            using R = decltype(callable());
            if constexpr (std::is_void_v<R>) {
                auto promise = std::make_shared<std::promise<void>>();
                auto task = this->EnqueueCallback(
                    [promise, callable = std::move(callable)]() mutable {
                        try {
                            callable();
                            promise->set_value();
                        } catch (...) {
                            promise->set_exception(std::current_exception());
                        }
                    });
                return std::make_shared<FutureTask<R>>(std::move(task),
                                                       promise->get_future());
            } else {
                auto promise = std::make_shared<std::promise<R>>();
                auto task = this->EnqueueCallback(
                    [promise, callable = std::move(callable)]() mutable {
                        try {
                            promise->set_value(callable());
                        } catch (...) {
                            promise->set_exception(std::current_exception());
                        }
                    });
                return std::make_shared<FutureTask<R>>(std::move(task),
                                                       promise->get_future());
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