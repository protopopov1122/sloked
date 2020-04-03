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

#ifndef SLOKED_SCHED_ACTIONQUEUE_H_
#define SLOKED_SCHED_ACTIONQUEUE_H_

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>

#include "sloked/core/Closeable.h"

namespace sloked {

    class SlokedActionQueue {
     public:
        virtual ~SlokedActionQueue() = default;

        template <typename T>
        auto Enqueue(T callable) -> std::future<decltype(std::declval<T>()())> {
            using R = decltype(callable());
            if constexpr (std::is_void_v<R>) {
                auto promise = std::make_shared<std::promise<void>>();
                this->EnqueueCallback(
                    [promise, callable = std::move(callable)]() mutable {
                        try {
                            callable();
                            promise->set_value();
                        } catch (...) {
                            promise->set_exception(std::current_exception());
                        }
                    });
                return promise->get_future();
            } else {
                auto promise = std::make_shared<std::promise<R>>();
                this->EnqueueCallback(
                    [promise, callable = std::move(callable)]() mutable {
                        try {
                            promise->set_value(callable());
                        } catch (...) {
                            promise->set_exception(std::current_exception());
                        }
                    });
                return promise->get_future();
            }
        }

     protected:
        virtual void EnqueueCallback(std::function<void()>) = 0;
    };

    class SlokedSingleThreadActionQueue : public SlokedActionQueue,
                                          public SlokedCloseable {
     public:
        SlokedSingleThreadActionQueue();
        ~SlokedSingleThreadActionQueue();
        void Start();
        void Close() final;

     protected:
        void EnqueueCallback(std::function<void()>) final;

     private:
        void Run();

        std::queue<std::function<void()>> queue;
        std::mutex mtx;
        std::condition_variable cv;
        std::atomic_bool active;
        std::thread worker;
    };
}  // namespace sloked

#endif