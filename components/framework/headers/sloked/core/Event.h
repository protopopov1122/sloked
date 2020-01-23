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

#ifndef SLOKED_CORE_EVENT_H_
#define SLOKED_CORE_EVENT_H_

#include "sloked/Base.h"
#include <functional>
#include <map>
#include <mutex>

namespace sloked {

    template <typename T>
    class SlokedEventEmitter {
     public:
        using Listener = std::function<void(T)>;
        using Unbind = std::function<void()>;

        SlokedEventEmitter()
            : nextId{0} {}

        Unbind Listen(Listener listener) {
            std::unique_lock lock(this->mtx);
            auto id = this->nextId++;
            this->listeners.emplace(id, std::move(listener));
            return [this, id] {
                std::unique_lock lock(this->mtx);
                if (this->listeners.count(id) != 0) {
                    this->listeners.erase(id);
                }
            };
        }

        void Emit(T &&event) {
            std::unique_lock lock(this->mtx);
            for (auto it = this->listeners.begin(); it != this->listeners.end();) {
                auto current = it++;
                lock.unlock();
                current->second(std::forward<T>(event));
                lock.lock();
            }
        }

     private:
        std::mutex mtx;
        uint64_t nextId;
        std::map<uint64_t, Listener> listeners;
    };

    template <>
    class SlokedEventEmitter<void> {
     public:
        using Listener = std::function<void()>;
        using Unbind = std::function<void()>;

        SlokedEventEmitter()
            : nextId{0} {}

        Unbind Listen(Listener listener) {
            std::unique_lock lock(this->mtx);
            auto id = this->nextId++;
            this->listeners.emplace(id, std::move(listener));
            return [this, id] {
                std::unique_lock lock(this->mtx);
                if (this->listeners.count(id) != 0) {
                    this->listeners.erase(id);
                }
            };
        }

        void Emit() {
            std::unique_lock lock(this->mtx);
            for (auto it = this->listeners.begin(); it != this->listeners.end();) {
                auto current = it++;
                lock.unlock();
                current->second();
                lock.lock();
            }
        }

     private:
        std::mutex mtx;
        uint64_t nextId;
        std::map<uint64_t, Listener> listeners;
    };
}

#endif