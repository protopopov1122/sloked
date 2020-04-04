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

#include "sloked/sched/SequentialExecutor.h"

namespace sloked {

    SlokedSequentialExecutor::SlokedSequentialExecutor() : active{false} {}

    SlokedSequentialExecutor::~SlokedSequentialExecutor() {
        this->Close();
    }

    void SlokedSequentialExecutor::Start() {
        if (!this->active.exchange(true)) {
            this->worker = std::thread([this] { this->Run(); });
        }
    }
    void SlokedSequentialExecutor::Close() {
        if (this->active.exchange(false) && this->worker.joinable()) {
            this->cv.notify_all();
            this->worker.join();
            std::unique_lock lock(this->mtx);
            for (auto task : this->queue) {
                task->Cancel();
            }
        }
    }

    std::shared_ptr<SlokedExecutor::Task>
        SlokedSequentialExecutor::EnqueueCallback(std::function<void()> exec) {
        std::unique_lock lock(this->mtx);
        auto task = std::make_shared<SlokedRunnableTask>(
            [this](auto &task) {
                std::unique_lock lock(this->mtx);
                this->queue.erase(
                    std::remove_if(this->queue.begin(), this->queue.end(),
                                   [&task](auto &other) {
                                       return std::addressof(task) ==
                                              other.get();
                                   }),
                    this->queue.end());
            },
            std::move(exec));
        this->queue.emplace_back(task);
        this->cv.notify_all();
        return task;
    }

    void SlokedSequentialExecutor::Run() {
        constexpr std::chrono::milliseconds Timeout{10};
        std::unique_lock lock(this->mtx);
        while (this->active.load()) {
            while (this->active.load() && !this->queue.empty()) {
                auto task = std::move(this->queue.front());
                this->queue.pop_front();
                lock.unlock();
                task->Start();
                lock.lock();
            }
            while (this->active.load() && this->queue.empty()) {
                this->cv.wait_for(lock, Timeout);
            }
        }
    }
}  // namespace sloked