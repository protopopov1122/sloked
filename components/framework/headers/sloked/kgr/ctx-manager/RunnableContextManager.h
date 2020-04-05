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

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>

#include "sloked/core/Closeable.h"
#include "sloked/core/Runnable.h"
#include "sloked/kgr/ContextManager.h"

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
        KgrRunnableContextManagerHandle() : work(false), notifications(0) {
            this->manager.SetActivationListener([&]() {
                std::unique_lock<std::mutex> lock(this->notificationMutex);
                this->notifications++;
                this->notificationCV.notify_all();
            });
        }

        ~KgrRunnableContextManagerHandle() {
            this->Close();
        }

        KgrContextManager<T> &GetManager() {
            return this->manager;
        }

        void Start() {
            if (this->work.exchange(true)) {
                return;
            }
            this->managerThread = std::thread([&]() {
                while (this->work.load()) {
                    std::unique_lock<std::mutex> notificationLock(
                        this->notificationMutex);
                    while (!this->manager.HasPendingActions() &&
                           this->work.load() &&
                           this->notifications.load() == 0) {
                        this->notificationCV.wait(notificationLock);
                    }
                    this->notifications = 0;
                    notificationLock.unlock();
                    this->manager.Run();
                }
            });
        }

        void Close() final {
            if (this->work.exchange(false) && this->managerThread.joinable()) {
                this->notificationCV.notify_all();
                this->managerThread.join();
                this->manager.Clear();
            }
        }

     private:
        KgrRunnableContextManager<T> manager;
        std::atomic<bool> work;
        std::atomic<uint32_t> notifications;
        std::mutex notificationMutex;
        std::condition_variable notificationCV;
        std::thread managerThread;
    };
}  // namespace sloked

#endif