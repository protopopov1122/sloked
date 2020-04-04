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

#include "sloked/sched/MultithreadExecutor.h"

#include <chrono>
#include <thread>

#include "sloked/core/Error.h"

namespace sloked {

    static constexpr std::chrono::seconds PollTimeout{5};
    static constexpr unsigned int MaxInactivity = 12;

    SlokedMultitheadExecutor::SlokedMultitheadExecutor(std::size_t max_workers)
        : active{true}, max_workers{max_workers}, total_workers{0},
          available_workers{0} {}

    SlokedMultitheadExecutor::~SlokedMultitheadExecutor() {
        this->Close();
    }

    std::shared_ptr<SlokedExecutor::Task>
        SlokedMultitheadExecutor::EnqueueCallback(
            std::function<void()> callback) {
        std::unique_lock lock(this->task_mtx);
        if (!this->active.load()) {
            throw SlokedError("ThreadManager: Shutting down");
        }
        auto task = std::make_shared<SlokedRunnableTask>(
            [this](auto &task) {
                std::unique_lock lock(this->task_mtx);
                this->pending.erase(
                    std::remove_if(this->pending.begin(), this->pending.end(),
                                   [&task](auto &other) {
                                       return std::addressof(task) ==
                                              other.get();
                                   }),
                    this->pending.end());
            },
            std::move(callback));
        this->pending.emplace_back(task);
        if ((this->total_workers.Load() < this->max_workers ||
             this->max_workers == 0) &&
            this->available_workers == 0) {
            this->SpawnWorker();
        }
        this->task_cv.notify_all();
        return task;
    }

    void SlokedMultitheadExecutor::Close() {
        this->active = false;
        this->task_cv.notify_all();
        this->total_workers.Wait([](auto count) { return count == 0; });
        std::unique_lock lock(this->task_mtx);
        for (auto task : this->pending) {
            task->Cancel();
        }
    }

    void SlokedMultitheadExecutor::SpawnWorker() {
        std::thread([this] {
            this->available_workers++;
            try {
                this->ProcessWorker();
            } catch (...) {
                this->available_workers--;
                this->total_workers.Decrement();
                throw;
            }
            this->available_workers--;
            this->total_workers.Decrement();
        }).detach();
        this->total_workers.Increment();
    }

    void SlokedMultitheadExecutor::ProcessWorker() {
        for (unsigned int counter = 0;
             this->active.load() && counter < MaxInactivity; counter++) {
            std::unique_lock lock(this->task_mtx);
            if (!this->pending.empty()) {
                this->available_workers--;
                counter = 0;
                auto task = std::move(this->pending.front());
                this->pending.pop_front();
                lock.unlock();
                task->Start();
                lock.lock();
                this->available_workers++;
            }
            if (!this->active ||
                this->available_workers >= this->total_workers.Load()) {
                break;
            }
            this->task_cv.wait_for(lock, PollTimeout);
        }
    }
}  // namespace sloked