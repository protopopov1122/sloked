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

#include "sloked/sched/Scheduler.h"
#include <thread>

namespace sloked {

    bool SlokedDefaultSchedulerThread::TimerTask::Pending() const {
        return this->pending.load();
    }

    const SlokedDefaultSchedulerThread::TimePoint &SlokedDefaultSchedulerThread::TimerTask::GetTime() const {
        return this->at;
    }

    void SlokedDefaultSchedulerThread::TimerTask::Run() {
        this->pending = this->interval.has_value();
        this->callback();
    }

    void SlokedDefaultSchedulerThread::TimerTask::Cancel() {
        this->pending = false;
        std::unique_lock lock(this->sched.mtx);
        this->sched.tasks.erase(this);
        this->sched.cv.notify_all();
    }

    SlokedDefaultSchedulerThread::TimerTask::TimerTask(SlokedDefaultSchedulerThread &sched, TimePoint at, Callback callback, std::optional<TimeDiff> interval)
        : sched(sched), pending(true), at(std::move(at)), callback(std::move(callback)), interval(std::move(interval)) {}

    void SlokedDefaultSchedulerThread::TimerTask::NextInterval() {
        if (this->pending.load()) {
            this->at = std::chrono::system_clock::now();
            this->at += this->interval.value();
        }
    }

    SlokedDefaultSchedulerThread::SlokedDefaultSchedulerThread()
        : work(false) {}

    void SlokedDefaultSchedulerThread::Start() {
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

    void SlokedDefaultSchedulerThread::Stop() {
        if (this->work.exchange(false)) {
            this->cv.notify_all();
            this->timer_thread.Wait([](auto count) {
                return count == 0;
            });
        }
    }

    std::shared_ptr<SlokedSchedulerThread::TimerTask> SlokedDefaultSchedulerThread::At(TimePoint time, Callback callback) {
        std::unique_lock lock(this->mtx);
        std::shared_ptr<TimerTask> task(new TimerTask(*this, std::move(time), std::move(callback)));
        this->tasks.emplace(task.get(), task);
        this->cv.notify_all();
        return task;
    }

    std::shared_ptr<SlokedSchedulerThread::TimerTask> SlokedDefaultSchedulerThread::Sleep(TimeDiff diff, Callback callback) {
        auto now = std::chrono::system_clock::now();
        now += diff;
        return this->At(now, std::move(callback));
    }

    std::shared_ptr<SlokedSchedulerThread::TimerTask> SlokedDefaultSchedulerThread::Interval(TimeDiff diff, Callback callback) {
        auto now = std::chrono::system_clock::now();
        now += diff;
        std::unique_lock lock(this->mtx);
        std::shared_ptr<TimerTask> task(new TimerTask(*this, std::move(now), std::move(callback), std::move(diff)));
        this->tasks.emplace(task.get(), task);
        this->cv.notify_all();
        return task;
    }

    void SlokedDefaultSchedulerThread::Defer(std::function<void()> callback) {
        std::unique_lock lock(this->mtx);
        this->deferred.push(std::move(callback));
        this->cv.notify_all();
    }

    bool SlokedDefaultSchedulerThread::TimerTaskCompare::operator()(TimerTask *t1, TimerTask *t2) const {
        return t1 != t2 && t1->GetTime() < t2->GetTime();
    }

    void SlokedDefaultSchedulerThread::Run() {
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
                std::shared_ptr<TimerTask> taskHandle = task->second;
                this->tasks.erase(task->first);
                taskHandle->NextInterval();
                this->tasks[taskHandle.get()] = taskHandle;
            }
        }
        while (this->work.load() && !this->deferred.empty()) {
            auto task = std::move(this->deferred.front());
            this->deferred.pop();
            lock.unlock();
            task();
            lock.lock();
        }
        if (this->work.load()) {
            if (this->tasks.empty()) {
                this->cv.wait(lock);
            } else {
                this->cv.wait_for(lock, (this->tasks.begin()->second)->GetTime() - now);
            }
        }
    }

    SlokedScheduledTaskPool::SlokedScheduledTaskPool(SlokedSchedulerThread &sched)
        : sched(sched) {}

    SlokedScheduledTaskPool::~SlokedScheduledTaskPool() {
        this->DropAll();
    }

    std::shared_ptr<SlokedSchedulerThread::TimerTask> SlokedScheduledTaskPool::At(TimePoint tp, Callback callback) {
        auto task = this->sched.At(tp, std::move(callback));
        std::unique_lock lock(this->mtx);
        this->tasks.push_back(task);
        return task;
    }

    std::shared_ptr<SlokedSchedulerThread::TimerTask> SlokedScheduledTaskPool::Sleep(TimeDiff td, Callback callback) {
        auto task = this->sched.Sleep(td, std::move(callback));
        std::unique_lock lock(this->mtx);
        this->tasks.push_back(task);
        return task;
    }

    std::shared_ptr<SlokedSchedulerThread::TimerTask> SlokedScheduledTaskPool::Interval(TimeDiff td, Callback callback) {
        auto task = this->sched.Interval(td, std::move(callback));
        std::unique_lock lock(this->mtx);
        this->tasks.push_back(task);
        return task;
    }

    void SlokedScheduledTaskPool::Defer(std::function<void()> callback) {
        this->sched.Defer(std::move(callback));
    }

    void SlokedScheduledTaskPool::CollectGarbage() {
        std::unique_lock lock(this->mtx);
        for (auto it = this->tasks.begin(); it != this->tasks.end();) {
            auto current = it++;
            if (!(*current)->Pending()) {
                this->tasks.erase(current);
            }
        }
    }

    void SlokedScheduledTaskPool::DropAll() {
        std::unique_lock lock(this->mtx);
        for (const auto &it : this->tasks) {
            it->Cancel();
        }
        this->tasks.clear();
    }
}