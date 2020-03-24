/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#include "sloked/sched/ThreadManager.h"
#include "sloked/core/Error.h"
#include <chrono>
#include <thread>

namespace sloked {

    static constexpr std::chrono::seconds PollTimeout{5};
    static constexpr unsigned int MaxInactivity = 12;

    SlokedDefaultThreadManager::SlokedDefaultThreadManager(std::size_t max_workers)
        : active{true}, max_workers{max_workers}, total_workers{0}, available_workers{0} {}

    SlokedDefaultThreadManager::~SlokedDefaultThreadManager() {
        this->Shutdown();
    }

    void SlokedDefaultThreadManager::Spawn(Task task) {
        std::unique_lock lock(this->task_mtx);
        if (!this->active.load()) {
            throw SlokedError("ThreadManager: Shutting down");
        }
        this->pending.push(std::move(task));
        if ((this->total_workers.Load() < this->max_workers || this->max_workers == 0) && this->available_workers == 0) {
            this->SpawnWorker();
        }
        this->task_cv.notify_all();
    }

    void SlokedDefaultThreadManager::Shutdown() {
        this->active = false;
        this->task_cv.notify_all();
        this->total_workers.Wait([](auto count) { return count == 0; });
    }

    void SlokedDefaultThreadManager::SpawnWorker() {
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

    void SlokedDefaultThreadManager::ProcessWorker() {
        for (unsigned int counter = 0; this->active.load() && counter < MaxInactivity; counter++) {
            std::unique_lock lock(this->task_mtx);
            if (!this->pending.empty()) {
                this->available_workers--;
                counter = 0;
                auto task = std::move(this->pending.front());
                this->pending.pop();
                lock.unlock();
                task();
                lock.lock();
                this->available_workers++;
            }
            if (!this->active) {
                break;
            }
            this->task_cv.wait_for(lock, PollTimeout);
        }
    }
}