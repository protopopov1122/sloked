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

#include "sloked/core/ThreadPool.h"
#include <thread>

namespace sloked {

    SlokedThreadPool::SlokedThreadPool()
        : workers{0} {}

    SlokedThreadPool::~SlokedThreadPool() {
        this->Wait();
    }
    void SlokedThreadPool::Start(std::function<void()> callback) {
        SlokedCounter<std::size_t>::Handle handle(this->workers);
        std::thread([callback = std::move(callback), handle = std::move(handle)] {
            callback();
        }).detach();
    }

    void SlokedThreadPool::Wait() {
        this->workers.Wait([](auto count) {
            return count == 0;
        });
    }
}