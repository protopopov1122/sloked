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

    bool SlokedTimerScheduler::Task::Pending() const {
        return this->pending.load();
    }

    const SlokedTimerScheduler::TimePoint &SlokedTimerScheduler::Task::GetTime() const {
        return this->at;
    }

    void SlokedTimerScheduler::Task::Run() {
        this->pending = this->interval.has_value();
        this->callback();
    }

    void SlokedTimerScheduler::Task::Cancel() {
        this->pending = false;
        std::unique_lock lock(this->sched.mtx);
        this->sched.tasks.erase(this);
        this->sched.cv.notify_all();
    }

    SlokedTimerScheduler::Task::Task(SlokedTimerScheduler &sched, TimePoint at, Callback callback, std::optional<TimeDiff> interval)
        : sched(sched), pending(true), at(std::move(at)), callback(std::move(callback)), interval(std::move(interval)) {}

    void SlokedTimerScheduler::Task::NextInterval() {
        if (this->pending.load()) {
            this->at = std::chrono::system_clock::now();
            this->at += this->interval.value();
        }
    }

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

    std::shared_ptr<SlokedTimerScheduler::Task> SlokedTimerScheduler::At(TimePoint time, Callback callback) {
        std::unique_lock lock(this->mtx);
        std::shared_ptr<Task> task(new Task(*this, std::move(time), std::move(callback)));
        this->tasks.emplace(task.get(), task);
        this->cv.notify_all();
        return task;
    }

    std::shared_ptr<SlokedTimerScheduler::Task> SlokedTimerScheduler::Sleep(TimeDiff diff, Callback callback) {
        auto now = std::chrono::system_clock::now();
        now += diff;
        return this->At(now, std::move(callback));
    }

    std::shared_ptr<SlokedTimerScheduler::Task> SlokedTimerScheduler::Interval(TimeDiff diff, Callback callback) {
        auto now = std::chrono::system_clock::now();
        now += diff;
        std::unique_lock lock(this->mtx);
        std::shared_ptr<Task> task(new Task(*this, std::move(now), std::move(callback), std::move(diff)));
        this->tasks.emplace(task.get(), task);
        this->cv.notify_all();
        return task;
    }

    bool SlokedTimerScheduler::TaskCompare::operator()(Task *t1, Task *t2) const {
        return t1 != t2 && t1->GetTime() < t2->GetTime();
    }

    void SlokedTimerScheduler::Run() {
        std::unique_lock lock(this->mtx);
        auto now = std::chrono::system_clock::now();
        while (this->work.load() && !this->tasks.empty() && (this->tasks.begin()->second)->GetTime() <= now) {
            auto task = this->tasks.begin();
            bool erase = true;
            if (task->second->Pending()) {
                lock.unlock();
                task->second->Run();
                erase = !task->second->Pending();
                lock.lock();
            }
            if (erase) {
                this->tasks.erase(task->first);
            } else {
                std::shared_ptr<Task> taskHandle = task->second;
                this->tasks.erase(task->first);
                taskHandle->NextInterval();
                this->tasks[taskHandle.get()] = taskHandle;
            }
        }
        if (this->tasks.empty()) {
            this->cv.wait(lock);
        } else {
            this->cv.wait_for(lock, (this->tasks.begin()->second)->GetTime() - now);
        }
    }
}