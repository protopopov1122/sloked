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

#include "sloked/sched/ScopedScheduler.h"

namespace sloked {

    SlokedScopedScheduler::ScopedTimerTask::ScopedTimerTask(
        SlokedScopedScheduler &sched, std::shared_ptr<TimerTask> task,
        std::size_t id)
        : sched(sched), task(std::move(task)), id(id) {}

    SlokedExecutor::State SlokedScopedScheduler::ScopedTimerTask::Status()
        const {
        return this->task->Status();
    }

    void SlokedScopedScheduler::ScopedTimerTask::Wait() {
        this->task->Wait();
    }

    void SlokedScopedScheduler::ScopedTimerTask::Cancel() {
        this->task->Cancel();
        std::unique_lock lock(this->sched.mtx);
        this->sched.garbage.push_back(this->sched.tasks.at(this->id));
        this->sched.tasks.erase(this->id);
        this->sched.CollectGarbage();
    }

    SlokedScheduler::TimePoint SlokedScopedScheduler::ScopedTimerTask::GetTime()
        const {
        return this->task->GetTime();
    }

    bool SlokedScopedScheduler::ScopedTimerTask::IsRecurring() const {
        return this->task->IsRecurring();
    }

    std::optional<SlokedScheduler::TimeDiff>
        SlokedScopedScheduler::ScopedTimerTask::GetInterval() const {
        return this->task->GetInterval();
    }

    SlokedScopedScheduler::SlokedScopedScheduler(SlokedScheduler &sched)
        : sched(sched), active{true}, nextId{0} {}

    SlokedScopedScheduler::~SlokedScopedScheduler() {
        this->Close();
    }

    void SlokedScopedScheduler::Close() {
        std::unique_lock lock(this->mtx);
        this->active = false;
        for (auto it = this->tasks.begin(); it != this->tasks.end();) {
            auto task = (it++)->second;
            lock.unlock();
            task->Cancel();
            lock.lock();
        }
        this->tasks.clear();
        constexpr std::chrono::milliseconds Timeout{10};
        while (!this->garbage.empty()) {
            this->CollectGarbage();
            this->cv.wait_for(lock, Timeout);
        }
    }

    std::shared_ptr<SlokedScheduler::TimerTask>
        SlokedScopedScheduler::EnqueueAt(TimePoint tp, Callback callback) {
        std::unique_lock lock(this->mtx);
        if (!this->active) {
            return nullptr;
        }
        auto id = this->nextId++;
        auto task = this->sched.At(
            tp, [this, id, callback = std::move(callback)]() mutable {
                callback();
                std::unique_lock lock(this->mtx);
                this->garbage.push_back(this->tasks.at(id));
                this->tasks.erase(id);
                this->CollectGarbage();
            });
        auto scopedTask =
            std::make_shared<ScopedTimerTask>(*this, std::move(task), id);
        this->tasks.insert_or_assign(id, scopedTask);
        this->CollectGarbage();
        return scopedTask;
    }

    std::shared_ptr<SlokedScheduler::TimerTask>
        SlokedScopedScheduler::EnqueueSleep(TimeDiff td, Callback callback) {
        std::unique_lock lock(this->mtx);
        if (!this->active) {
            return nullptr;
        }
        auto id = this->nextId++;
        auto task = this->sched.Sleep(
            td, [this, id, callback = std::move(callback)]() mutable {
                callback();
                std::unique_lock lock(this->mtx);
                this->garbage.push_back(this->tasks.at(id));
                this->tasks.erase(id);
                this->CollectGarbage();
            });
        auto scopedTask =
            std::make_shared<ScopedTimerTask>(*this, std::move(task), id);
        this->tasks.insert_or_assign(id, scopedTask);
        this->CollectGarbage();
        return scopedTask;
    }

    std::shared_ptr<SlokedScheduler::TimerTask>
        SlokedScopedScheduler::EnqueueInterval(TimeDiff td, Callback callback) {
        std::unique_lock lock(this->mtx);
        if (!this->active) {
            return nullptr;
        }
        auto id = this->nextId++;
        auto task = this->sched.Sleep(
            td, [callback = std::move(callback)]() mutable { callback(); });
        auto scopedTask =
            std::make_shared<ScopedTimerTask>(*this, std::move(task), id);
        this->tasks.insert_or_assign(id, scopedTask);
        this->CollectGarbage();
        return scopedTask;
    }

    void SlokedScopedScheduler::CollectGarbage() {
        this->garbage.erase(
            std::remove_if(this->garbage.begin(), this->garbage.end(),
                           [](auto &task) { return task->Complete(); }),
            this->garbage.end());
        this->cv.notify_all();
    }
}  // namespace sloked