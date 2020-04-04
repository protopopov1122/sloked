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

#include "sloked/sched/DefaultScheduler.h"

namespace sloked {

    SlokedExecutor::State SlokedDefaultScheduler::TimerTask::Status() const {
        return this->state.load();
    }

    void SlokedDefaultScheduler::TimerTask::Wait() {
        std::unique_lock lock(this->mtx);
        this->cv.wait(lock, [this] {
            return state == SlokedExecutor::State::Finished ||
                   state == SlokedExecutor::State::Canceled;
        });
    }

    void SlokedDefaultScheduler::TimerTask::Cancel() {
        std::unique_lock lock(this->mtx);
        if (this->state == SlokedExecutor::State::Pending ||
            this->state == SlokedExecutor::State::Running) {
            this->state = SlokedExecutor::State::Canceled;
            this->cancel(*this);
            this->cv.notify_all();
        }
    }

    SlokedDefaultScheduler::TimePoint
        SlokedDefaultScheduler::TimerTask::GetTime() const {
        return this->at.load();
    }

    bool SlokedDefaultScheduler::TimerTask::IsRecurring() const {
        return this->interval.has_value();
    }

    std::optional<SlokedScheduler::TimeDiff>
        SlokedDefaultScheduler::TimerTask::GetInterval() const {
        return this->interval;
    }

    void SlokedDefaultScheduler::TimerTask::Start() {
        std::unique_lock lock(this->mtx);
        if (this->state != SlokedExecutor::State::Pending) {
            return;
        }
        this->state = SlokedExecutor::State::Running;
        lock.unlock();
        this->callback();
        lock.lock();
        if (this->interval.has_value()) {
            if (this->state != SlokedExecutor::State::Canceled) {
                this->state = SlokedExecutor::State::Pending;
            }
        } else {
            this->state = SlokedExecutor::State::Finished;
        }
        this->cv.notify_all();
    }

    SlokedDefaultScheduler::TimerTask::TimerTask(
        std::function<void(TimerTask &)> cancel, TimePoint at,
        Callback callback, std::optional<TimeDiff> interval)
        : cancel(cancel), state(SlokedExecutor::State::Pending),
          at(std::move(at)), callback(std::move(callback)),
          interval(std::move(interval)) {}

    void SlokedDefaultScheduler::TimerTask::NextInterval() {
        std::unique_lock lock(this->mtx);
        if (this->state == SlokedExecutor::State::Pending) {
            this->at =
                std::chrono::system_clock::now() + this->interval.value();
        }
    }

    SlokedDefaultScheduler::SlokedDefaultScheduler(SlokedExecutor &executor)
        : executor(executor), work(false) {}

    SlokedDefaultScheduler::~SlokedDefaultScheduler() {
        this->Close();
    }

    void SlokedDefaultScheduler::Start() {
        if (!this->work.exchange(true)) {
            this->worker = std::thread([this] {
                while (this->work.load()) {
                    this->Run();
                }
            });
        }
    }

    void SlokedDefaultScheduler::Close() {
        if (this->work.exchange(false) && this->worker.joinable()) {
            this->cv.notify_all();
            this->worker.join();
        }
    }

    std::shared_ptr<SlokedScheduler::TimerTask>
        SlokedDefaultScheduler::EnqueueAt(TimePoint time, Callback callback) {
        std::unique_lock lock(this->mtx);
        std::shared_ptr<TimerTask> task(
            new TimerTask([this](auto &task) { this->EraseTask(task); },
                          std::move(time), std::move(callback)));
        this->tasks.insert_or_assign(task.get(), task);
        this->cv.notify_all();
        return task;
    }

    std::shared_ptr<SlokedScheduler::TimerTask>
        SlokedDefaultScheduler::EnqueueSleep(TimeDiff diff, Callback callback) {
        auto now = std::chrono::system_clock::now();
        now += diff;
        return this->At(now, std::move(callback));
    }

    std::shared_ptr<SlokedScheduler::TimerTask>
        SlokedDefaultScheduler::EnqueueInterval(TimeDiff diff,
                                                Callback callback) {
        auto now = std::chrono::system_clock::now();
        now += diff;
        std::unique_lock lock(this->mtx);
        std::shared_ptr<TimerTask> task(new TimerTask(
            [this](auto &task) { this->EraseTask(task); }, std::move(now),
            std::move(callback), std::move(diff)));
        this->tasks.emplace(task.get(), task);
        this->cv.notify_all();
        return task;
    }

    bool SlokedDefaultScheduler::TimerTaskCompare::operator()(
        TimerTask *t1, TimerTask *t2) const {
        return t1 != t2 && t1->GetTime() < t2->GetTime();
    }

    void SlokedDefaultScheduler::Run() {
        std::unique_lock lock(this->mtx);
        auto now = std::chrono::system_clock::now();
        while (this->work.load() && !this->tasks.empty() &&
               (this->tasks.begin()->second)->GetTime() <= now) {
            auto taskIt = this->tasks.begin();
            auto taskId = taskIt->first;
            auto task = taskIt->second;
            this->tasks.erase(taskId);
            if (task->Pending()) {
                this->executor.Enqueue([this, taskId, task = std::move(task)] {
                    task->Start();
                    if (task->Pending()) {
                        std::unique_lock lock(this->mtx);
                        task->NextInterval();
                        this->tasks.emplace(task.get(), task);
                    }
                });
            }
        }
        if (this->work.load()) {
            if (this->tasks.empty()) {
                this->cv.wait(lock);
            } else {
                this->cv.wait_for(
                    lock, (this->tasks.begin()->second)->GetTime() - now);
            }
        }
    }

    void SlokedDefaultScheduler::EraseTask(TimerTask &task) {
        std::unique_lock lock(this->mtx);
        this->tasks.erase(std::addressof(task));
        this->cv.notify_all();
    }
}  // namespace sloked