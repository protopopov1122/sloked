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

#ifndef SLOKED_SCHED_SEQUENTIALEXECUTOR_H_
#define SLOKED_SCHED_SEQUENTIALEXECUTOR_H_

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>

#include "sloked/sched/Executor.h"

namespace sloked {

    class SlokedSequentialExecutor : public SlokedExecutor,
                                     public SlokedCloseable {
     public:
        SlokedSequentialExecutor();
        ~SlokedSequentialExecutor();
        void Start();
        void Close() final;

     protected:
        std::shared_ptr<Task> EnqueueCallback(std::function<void()>) final;

     private:
        void Run();

        std::list<std::shared_ptr<SlokedRunnableTask>> queue;
        std::mutex mtx;
        std::condition_variable cv;
        std::atomic_bool active;
        std::thread worker;
    };
}  // namespace sloked

#endif