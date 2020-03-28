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

#ifndef SLOKED_CORE_COUNTER_H_
#define SLOKED_CORE_COUNTER_H_

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <utility>

#include "sloked/Base.h"

namespace sloked {

    template <typename T>
    class SlokedCounter {
     public:
        SlokedCounter() : counter(T{0}) {}

        SlokedCounter(T value) : counter(value) {}

        void Increment() {
            this->counter++;
            this->cv.notify_all();
        }

        void Decrement() {
            this->counter--;
            this->cv.notify_all();
        }

        void Wait(std::function<bool(T)> cond) {
            std::unique_lock<std::mutex> lock(mutex);
            if (!cond(this->counter.load())) {
                this->cv.wait(lock, [&] { return cond(this->counter.load()); });
            }
        }

        T Load() {
            return this->counter.load();
        }

        class Handle {
         public:
            Handle(SlokedCounter<T> &counter) : counter(&counter) {
                this->counter->Increment();
            }

            Handle(const Handle &handle) : counter(handle.counter) {
                if (this->counter) {
                    this->counter->Increment();
                }
            }

            Handle(Handle &&handle) {
                this->counter = handle.counter;
                handle.counter = nullptr;
            }

            ~Handle() {
                if (this->counter) {
                    this->counter->Decrement();
                }
            }

            Handle &operator=(const Handle &handle) {
                if (this->counter) {
                    this->counter->Decrement();
                }
                this->counter = handle.counter;
                if (this->counter) {
                    this->counter->Increment();
                }
                return *this;
            }

            Handle &operator=(Handle &&handle) {
                if (this->counter) {
                    this->counter->Decrement();
                }
                this->counter = handle.counter;
                handle.counter = nullptr;
            }

            void Reset() {
                if (this->counter) {
                    this->counter->Decrement();
                    this->counter = nullptr;
                }
            }

         private:
            SlokedCounter<T> *counter;
        };

     private:
        std::atomic<T> counter;
        std::mutex mutex;
        std::condition_variable cv;
    };
}  // namespace sloked

#endif