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

#ifndef SLOKED_SCHED_DEFAULTSCHEDULER_H_
#define SLOKED_SCHED_DEFAULTSCHEDULER_H_

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "sloked/sched/Scheduler.h"

namespace sloked {

    class SlokedDefaultScheduler : public SlokedScheduler,
                                   public SlokedCloseable {
     public:
        class TimerTask : public SlokedScheduler::TimerTask {
         public:
            friend class SlokedDefaultScheduler;

            SlokedExecutor::State Status() const final;
            void Wait() final;
            void Cancel() final;
            TimePoint GetTime() const final;
            bool IsRecurring() const final;
            std::optional<TimeDiff> GetInterval() const final;

         private:
            TimerTask(std::function<void(TimerTask &)>, TimePoint, Callback,
                      std::optional<TimeDiff> = {});
            void Start();
            void NextInterval();

            std::function<void(TimerTask &)> cancel;
            std::atomic<SlokedExecutor::State> state;
            std::atomic<TimePoint> at;
            Callback callback;
            std::optional<TimeDiff> interval;
            mutable std::mutex mtx;
            std::condition_variable cv;
        };
        friend class TimerTask;

        SlokedDefaultScheduler(SlokedExecutor &);
        ~SlokedDefaultScheduler();
        void Start();
        void Close() final;

     protected:
        std::shared_ptr<SlokedScheduler::TimerTask> EnqueueAt(TimePoint,
                                                              Callback) final;
        std::shared_ptr<SlokedScheduler::TimerTask> EnqueueSleep(
            TimeDiff, Callback) final;
        std::shared_ptr<SlokedScheduler::TimerTask> EnqueueInterval(
            TimeDiff, Callback) final;

     private:
        struct TimerTaskCompare {
            bool operator()(TimerTask *, TimerTask *) const;
        };

        void Run();
        void EraseTask(TimerTask &);

        SlokedExecutor &executor;
        std::map<TimerTask *, std::shared_ptr<TimerTask>, TimerTaskCompare>
            tasks;
        std::atomic<bool> work;
        std::thread worker;
        std::mutex mtx;
        std::condition_variable cv;
    };
}  // namespace sloked

#endif