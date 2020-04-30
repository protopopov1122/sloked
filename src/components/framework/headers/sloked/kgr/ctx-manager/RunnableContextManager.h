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

#ifndef SLOKED_KGR_LOCAL_CTX_MANAGER_RUNNABLECONTEXTMANAGER_H_
#define SLOKED_KGR_LOCAL_CTX_MANAGER_RUNNABLECONTEXTMANAGER_H_

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <list>
#include <map>
#include <mutex>

#include "sloked/core/Closeable.h"
#include "sloked/core/Runnable.h"
#include "sloked/kgr/ContextManager.h"
#include "sloked/sched/Executor.h"

namespace sloked {

    template <typename T>
    class KgrRunnableContextManager : public KgrContextManager<T>,
                                      public SlokedRunnable {
     public:
        bool HasPendingActions() const {
            for (auto &ctx : this->contexts) {
                if (ctx->GetState() != KgrServiceContext::State::Idle) {
                    return true;
                }
            }
            return false;
        }

        void SetActivationListener(std::function<void()> callback) {
            std::unique_lock<std::mutex> lock(this->contexts_mtx);
            for (auto &ctx : this->contexts) {
                ctx->SetActivationListener(callback);
            }
            this->callback = std::move(callback);
        }

        void Clear() {
            std::unique_lock<std::mutex> lock(this->contexts_mtx);
            this->contexts.clear();
        }

        void Attach(std::unique_ptr<T> ctx) override {
            std::unique_lock<std::mutex> lock(this->contexts_mtx);
            ctx->SetActivationListener(this->callback);
            this->contexts.push_back(std::move(ctx));
            if (this->callback) {
                this->callback();
            }
        }

        void Run() override {
            if (this->contexts.empty()) {
                return;
            }
            for (auto it = this->contexts.begin();;) {
                if ((*it)->GetState() == KgrServiceContext::State::Pending) {
                    (*it)->Run();
                }

                std::unique_lock<std::mutex> lock(this->contexts_mtx);
                auto current_it = it;
                ++it;
                auto state = (*current_it)->GetState();
                if (state == KgrServiceContext::State::Finished ||
                    state == KgrServiceContext::State::Destroyed) {
                    this->contexts.erase(current_it);
                }
                if (it == this->contexts.end()) {
                    break;
                }
            }
        }

     private:
        std::list<std::unique_ptr<T>> contexts;
        std::mutex contexts_mtx;
        std::function<void()> callback;
    };

    template <typename T>
    class KgrRunnableContextManagerHandle : public SlokedCloseable {
     public:
        KgrRunnableContextManagerHandle(SlokedExecutor &exec)
            : exec(exec), work(false) {
            this->manager.SetActivationListener([&]() {
                std::unique_lock lock(this->mtx);
                if (this->work) {
                    auto id = this->nextId++;
                    this->tasks.insert_or_assign(
                        id, this->exec.Enqueue([this, id] { this->Run(id); }));
                }
            });
        }

        ~KgrRunnableContextManagerHandle() {
            this->Close();
        }

        KgrContextManager<T> &GetManager() {
            return this->manager;
        }

        void Start() {
            std::unique_lock lock(this->mtx);
            if (!this->work) {
                this->work = true;
                this->nextId = 0;
            }
        }

        void Close() final {
            std::unique_lock lock(this->mtx);
            if (this->work) {
                this->work = false;
                for (const auto &task : this->tasks) {
                    task.second->Cancel();
                }
                this->cv.wait(lock, [&] {
                    return this->tasks.empty() ||
                           std::all_of(this->tasks.begin(), this->tasks.end(),
                                       [](const auto &task) {
                                           return task.second->Complete();
                                       });
                });
                this->tasks.clear();
                this->manager.Clear();
            }
        }

     private:
        void Run(std::size_t id) {
            std::unique_lock lock(this->mtx);
            if (this->work) {
                lock.unlock();
                this->manager.Run();
                lock.lock();
            }
            this->tasks.erase(id);
            this->cv.notify_all();
        }

        KgrRunnableContextManager<T> manager;
        SlokedExecutor &exec;
        std::mutex mtx;
        std::condition_variable cv;
        bool work;
        std::size_t nextId;
        std::map<std::size_t, std::shared_ptr<SlokedExecutor::Task>> tasks;
    };
}  // namespace sloked

#endif