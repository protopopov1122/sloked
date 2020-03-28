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

#ifndef SLOKED_CORE_MONITOR_H_
#define SLOKED_CORE_MONITOR_H_

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>

#include "sloked/core/Error.h"

namespace sloked {

    template <typename T>
    class SlokedMonitor {
     public:
        SlokedMonitor() = delete;
        SlokedMonitor(T data) : data(std::forward<T>(data)) {}
        SlokedMonitor(const SlokedMonitor<T> &) = delete;
        SlokedMonitor(SlokedMonitor<T> &&) = default;

        SlokedMonitor &operator=(const SlokedMonitor<T> &) = delete;
        SlokedMonitor &operator=(SlokedMonitor<T> &&) = default;

        void Lock(std::function<void(T &)> callback) {
            std::unique_lock<std::mutex> lock(this->mtx);
            this->holder = std::this_thread::get_id();
            try {
                callback(this->data);
                this->holder = std::thread::id{};
            } catch (const SlokedError &err) {
                this->holder = std::thread::id{};
                throw;
            }
        }

        void Lock(std::function<void(const T &)> callback) const {
            std::unique_lock<std::mutex> lock(this->mtx);
            this->holder = std::this_thread::get_id();
            try {
                callback(this->data);
                this->holder = std::thread::id{};
            } catch (const SlokedError &err) {
                this->holder = std::thread::id{};
                throw;
            }
        }

        bool TryLock(std::function<void(T &)> callback) {
            std::unique_lock<std::mutex> lock(this->mtx, std::defer_lock);
            if (lock.try_lock()) {
                this->holder = std::this_thread::get_id();
                try {
                    callback(this->data);
                    this->holder = std::thread::id{};
                } catch (const SlokedError &err) {
                    this->holder = std::thread::id{};
                    throw;
                }
                return true;
            } else {
                return false;
            }
        }

        bool TryLock(std::function<void(const T &)> callback) const {
            std::unique_lock<std::mutex> lock(this->mtx, std::defer_lock);
            if (lock.try_lock()) {
                this->holder = std::this_thread::get_id();
                try {
                    callback(this->data);
                    this->holder = std::thread::id{};
                } catch (const SlokedError &err) {
                    this->holder = std::thread::id{};
                    throw;
                }
                return true;
            } else {
                return false;
            }
        }

        bool IsHolder() const {
            return this->holder.load() == std::this_thread::get_id();
        }

     private:
        T data;
        mutable std::mutex mtx;
        mutable std::atomic<std::thread::id> holder;
    };
}  // namespace sloked

#endif