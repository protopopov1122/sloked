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

#ifndef SLOKED_SCHED_SCOPEDEXECUTOR_H_
#define SLOKED_SCHED_SCOPEDEXECUTOR_H_

#include <map>
#include <mutex>

#include "sloked/sched/Executor.h"

namespace sloked {

    class SlokedScopedExecutor : public SlokedExecutor, public SlokedCloseable {
        class ScopedTask : public Task {
         public:
            ScopedTask(SlokedScopedExecutor &, std::size_t,
                       std::shared_ptr<Task>);

            State Status() const final;
            void Wait() final;
            void Cancel() final;

         private:
            SlokedScopedExecutor &executor;
            std::size_t id;
            std::shared_ptr<Task> task;
        };

        friend class ScopedTask;

     public:
        SlokedScopedExecutor(SlokedExecutor &);

        void Close() final;

     protected:
        std::shared_ptr<Task> EnqueueCallback(std::function<void()>) final;

     private:
        SlokedExecutor &executor;
        std::mutex mtx;
        std::size_t nextId;
        std::map<std::size_t, std::shared_ptr<ScopedTask>> tasks;
    };
}  // namespace sloked

#endif