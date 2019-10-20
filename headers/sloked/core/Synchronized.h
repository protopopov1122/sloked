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

#ifndef SLOKED_CORE_SYNCHRONIZED_H_
#define SLOKED_CORE_SYNCHRONIZED_H_

#include "sloked/Base.h"
#include <mutex>
#include <functional>

namespace sloked {

    template <typename T>
    class SlokedSynchronized {
     public:
        SlokedSynchronized() = delete;
        SlokedSynchronized(T data)
            : data(std::forward<T>(data)) {}
        SlokedSynchronized(const SlokedSynchronized<T> &) = delete;
        SlokedSynchronized(SlokedSynchronized<T> &&) = default;

        SlokedSynchronized &operator=(const SlokedSynchronized<T> &) = delete;
        SlokedSynchronized &operator=(SlokedSynchronized<T> &&) = default;

        void Lock(std::function<void(T &)> callback) {
            std::unique_lock<std::mutex> lock(this->mtx);
            callback(this->data);
        }

        void Lock(std::function<void(const T &)> callback) const {
            std::unique_lock<std::mutex> lock(this->mtx);
            callback(this->data);
        }

     private:
        T data;
        mutable std::mutex mtx;
    };
}

#endif