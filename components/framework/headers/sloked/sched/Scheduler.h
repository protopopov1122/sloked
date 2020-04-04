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

#ifndef SLOKED_SCHED_SCHEDULER_H_
#define SLOKED_SCHED_SCHEDULER_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>

#include "sloked/core/Closeable.h"
#include "sloked/core/Counter.h"
#include "sloked/sched/Executor.h"

namespace sloked {

    class SlokedSchedulerThread {
     public:
        using TimePoint =
            std::chrono::time_point<std::chrono::system_clock,
                                    std::chrono::system_clock::duration>;
        using TimeDiff = std::chrono::system_clock::duration;
        using Callback = std::function<void()>;

        class TimerTask {
         public:
            virtual ~TimerTask() = default;
            virtual bool Pending() const = 0;
            virtual const TimePoint &GetTime() const = 0;
            virtual void Cancel() = 0;
        };

        virtual ~SlokedSchedulerThread() = default;
        virtual std::shared_ptr<TimerTask> At(TimePoint, Callback) = 0;
        virtual std::shared_ptr<TimerTask> Sleep(TimeDiff, Callback) = 0;
        virtual std::shared_ptr<TimerTask> Interval(TimeDiff, Callback) = 0;
        virtual void Defer(std::function<void()>) = 0;
    };

    class SlokedDefaultScheduler : public SlokedSchedulerThread,
                                   public SlokedCloseable {
     public:
        class TimerTask : public SlokedSchedulerThread::TimerTask {
         public:
            friend class SlokedDefaultScheduler;

            void Run();

            bool Pending() const final;
            const TimePoint &GetTime() const final;
            void Cancel() final;

         private:
            TimerTask(SlokedDefaultScheduler &, TimePoint, Callback,
                      std::optional<TimeDiff> = {});
            void NextInterval();

            SlokedDefaultScheduler &sched;
            std::atomic<bool> pending;
            TimePoint at;
            Callback callback;
            std::optional<TimeDiff> interval;
        };
        friend class TimerTask;

        SlokedDefaultScheduler(SlokedExecutor &);
        ~SlokedDefaultScheduler();
        void Start();
        void Close() final;

        std::shared_ptr<SlokedSchedulerThread::TimerTask> At(TimePoint,
                                                             Callback) final;
        std::shared_ptr<SlokedSchedulerThread::TimerTask> Sleep(TimeDiff,
                                                                Callback) final;
        std::shared_ptr<SlokedSchedulerThread::TimerTask> Interval(
            TimeDiff, Callback) final;
        void Defer(std::function<void()>) final;

     private:
        struct TimerTaskCompare {
            bool operator()(TimerTask *, TimerTask *) const;
        };

        void Run();

        SlokedExecutor &executor;
        std::map<TimerTask *, std::shared_ptr<TimerTask>, TimerTaskCompare>
            tasks;
        std::atomic<bool> work;
        SlokedCounter<std::size_t> timer_thread;
        std::mutex mtx;
        std::condition_variable cv;
    };

    class SlokedScheduledTaskPool : public SlokedSchedulerThread {
     public:
        SlokedScheduledTaskPool(SlokedSchedulerThread &);
        ~SlokedScheduledTaskPool();
        std::shared_ptr<SlokedSchedulerThread::TimerTask> At(TimePoint,
                                                             Callback) final;
        std::shared_ptr<SlokedSchedulerThread::TimerTask> Sleep(TimeDiff,
                                                                Callback) final;
        std::shared_ptr<SlokedSchedulerThread::TimerTask> Interval(
            TimeDiff, Callback) final;
        void Defer(std::function<void()>) final;
        void CollectGarbage();
        void DropAll();

     private:
        SlokedSchedulerThread &sched;
        std::mutex mtx;
        std::list<std::shared_ptr<SlokedSchedulerThread::TimerTask>> tasks;
    };
}  // namespace sloked

#endif