/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

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

#ifndef SLOKED_SCHED_TIMER_H_
#define SLOKED_SCHED_TIMER_H_

#include "sloked/core/Counter.h"
#include <map>
#include <atomic>
#include <utility>
#include <chrono>
#include <functional>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>

namespace sloked {

    class SlokedTimerScheduler {
     public:
        using TimePoint = std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration>;
        using TimeDiff = std::chrono::system_clock::duration;
        using Callback = std::function<void()>;
        class Task {
         public:
            friend class SlokedTimerScheduler;

            bool Pending() const;
            const TimePoint &GetTime() const;
            void Run();
            void Cancel();

         private:
            Task(SlokedTimerScheduler &, TimePoint, Callback, std::optional<TimeDiff> = {});
            void NextInterval();

            SlokedTimerScheduler &sched;
            std::atomic<bool> pending;
            TimePoint at;
            Callback callback;
            std::optional<TimeDiff> interval;
        };
        friend class Task;

        SlokedTimerScheduler();
        void Start();
        void Stop();

        std::shared_ptr<Task> At(TimePoint, Callback);
        std::shared_ptr<Task> Sleep(TimeDiff, Callback);
        std::shared_ptr<Task> Interval(TimeDiff, Callback);

     private:
        struct TaskCompare {
            bool operator()(Task *, Task *) const;
        };

        void Run();

        std::map<Task *, std::shared_ptr<Task>, TaskCompare> tasks;
        std::atomic<bool> work;
        SlokedCounter<std::size_t> timer_thread;
        std::mutex mtx;
        std::condition_variable cv;
    };
}

#endif