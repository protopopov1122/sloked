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

#ifndef SLOKED_SCHED_MULTITHREADEXECUTOR_H_
#define SLOKED_SCHED_MULTITHREADEXECUTOR_H_

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <list>
#include <mutex>

#include "sloked/core/Counter.h"
#include "sloked/sched/Executor.h"

namespace sloked {

    class SlokedMultitheadExecutor : public SlokedExecutor,
                                     public SlokedCloseable {
     public:
        static constexpr std::size_t UnlimitedWorkers = 0;

        SlokedMultitheadExecutor(std::size_t = UnlimitedWorkers);
        ~SlokedMultitheadExecutor();

        void Close() final;

     private:
        std::shared_ptr<Task> EnqueueCallback(std::function<void()>) final;
        void SpawnWorker();
        void ProcessWorker();

        std::atomic_bool active;
        const std::size_t max_workers;
        std::mutex task_mtx;
        std::condition_variable task_cv;
        std::list<std::shared_ptr<SlokedRunnableTask>> pending;
        SlokedCounter<std::size_t> total_workers;
        std::size_t available_workers;
    };
}  // namespace sloked

#endif