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

#include "sloked/sched/Timer.h"
#include <thread>

namespace sloked {

    SlokedTimerScheduler::SlokedTimerScheduler()
        : work(false) {}

    void SlokedTimerScheduler::Start() {
        if (!this->work.exchange(true)) {
            std::thread([this] {
                SlokedCounter<std::size_t>::Handle counter(this->timer_thread);
                while (this->work.load()) {
                    this->Run();
                }
            }).detach();
            this->timer_thread.Wait([](auto count) {
                return count > 0;
            });
        }
    }

    void SlokedTimerScheduler::Stop() {
        if (this->work.exchange(false)) {
            this->cv.notify_all();
            this->timer_thread.Wait([](auto count) {
                return count == 0;
            });
        }
    }

    void SlokedTimerScheduler::At(TimePoint time, Callback callback) {
        std::unique_lock lock(this->mtx);
        this->tasks.push(std::make_pair(std::move(time), std::move(callback)));
        this->cv.notify_all();
    }

    void SlokedTimerScheduler::Sleep(TimeDiff diff, Callback callback) {
        auto now = std::chrono::system_clock::now();
        now += diff;
        this->At(now, std::move(callback));
    }

    bool SlokedTimerScheduler::TaskCompare::operator()(const Task &t1, const Task &t2) const {
        return t1.first >= t2.first;
    }

    void SlokedTimerScheduler::Run() {
        std::unique_lock lock(this->mtx);
        auto now = std::chrono::system_clock::now();
        while (this->work.load() && !this->tasks.empty() && this->tasks.top().first <= now) {
            auto task = std::move(this->tasks.top());
            this->tasks.pop();
            lock.unlock();
            task.second();
            lock.lock();
        }
        if (this->tasks.empty()) {
            this->cv.wait(lock);
        } else {
            this->cv.wait_for(lock, this->tasks.top().first - now);
        }
    }
}