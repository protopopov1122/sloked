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

#ifndef SLOKED_SCHED_SCOPEDSCHEDULER_H_
#define SLOKED_SCHED_SCOPEDSCHEDULER_H_

#include <condition_variable>
#include <list>
#include <map>
#include <mutex>

#include "sloked/sched/Scheduler.h"

namespace sloked {

    class SlokedScopedScheduler : public SlokedScheduler,
                                  public SlokedCloseable {
     public:
        class ScopedTimerTask : public TimerTask {
         public:
            ScopedTimerTask(SlokedScopedScheduler &, std::shared_ptr<TimerTask>,
                            std::size_t);

            SlokedExecutor::State Status() const final;
            void Wait() final;
            void Cancel() final;
            TimePoint GetTime() const final;
            bool IsRecurring() const final;
            std::optional<TimeDiff> GetInterval() const final;

         private:
            SlokedScopedScheduler &sched;
            std::shared_ptr<TimerTask> task;
            std::size_t id;
        };

        friend class ScopedTimerTask;

        SlokedScopedScheduler(SlokedScheduler &);
        ~SlokedScopedScheduler();
        void Close() final;

     protected:
        std::shared_ptr<SlokedScheduler::TimerTask> EnqueueAt(TimePoint,
                                                              Callback) final;
        std::shared_ptr<SlokedScheduler::TimerTask> EnqueueSleep(
            TimeDiff, Callback) final;
        std::shared_ptr<SlokedScheduler::TimerTask> EnqueueInterval(
            TimeDiff, Callback) final;

        void CollectGarbage();

     private:
        SlokedScheduler &sched;
        std::mutex mtx;
        std::condition_variable cv;
        bool active;
        std::size_t nextId;
        std::map<std::size_t, std::shared_ptr<ScopedTimerTask>> tasks;
        std::list<std::shared_ptr<ScopedTimerTask>> garbage;
    };
}  // namespace sloked

#endif