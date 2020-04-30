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

#ifndef SLOKED_CORE_SEMAPHORE_H_
#define SLOKED_CORE_SEMAPHORE_H_

#include <cinttypes>
#include <condition_variable>
#include <mutex>

#include "sloked/Base.h"

namespace sloked {

    class SlokedSemaphore {
     public:
        SlokedSemaphore();
        void Notify();
        void Wait();
        void WaitAll();

     private:
        std::mutex mtx;
        std::condition_variable cv;
        uint32_t counter;
    };
}  // namespace sloked

#endif