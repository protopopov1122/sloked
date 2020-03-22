/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_SCHED_THREADMANAGER_H_
#define SLOKED_SCHED_THREADMANAGER_H_

#include "sloked/core/Counter.h"
#include <functional>
#include <future>
#include <exception>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>

namespace sloked {

    class SlokedThreadManager {
     public:
        using Task = std::function<void()>;

        template <typename T>
        using Producer = std::function<T()>;

        virtual ~SlokedThreadManager() = default;
        virtual void Spawn(Task) = 0;
        virtual void Shutdown() = 0;
    
        template <typename T, typename E = std::enable_if_t<!std::is_void_v<T>>>
        std::future<T> Spawn(Producer<T> producer) {
            std::shared_ptr<std::promise<T>> promise = std::make_shared<std::promise<T>>();
            this->Spawn([producer = std::move(producer), promise] {
                try {
                    promise->set_value(producer());
                } catch (...) {
                    promise->set_exception(std::current_exception());
                }
            });
            return promise->get_future();
        }
    };

    class SlokedDefaultThreadManager : public SlokedThreadManager {
     public:
        static constexpr std::size_t UnlimitedWorkers = 0;

        SlokedDefaultThreadManager(std::size_t = UnlimitedWorkers);
        ~SlokedDefaultThreadManager();

        void Spawn(Task) final;
        void Shutdown() final;

     private:
        void SpawnWorker();
        void ProcessWorker();

        std::atomic_bool active;
        const std::size_t max_workers;
        std::mutex task_mtx;
        std::condition_variable task_cv;
        std::queue<Task> pending;
        SlokedCounter<std::size_t> total_workers;
        std::size_t available_workers;
    };
}

#endif