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

#ifndef SLOKED_CORE_AWAITABLE_POSIX_H_
#define SLOKED_CORE_AWAITABLE_POSIX_H_

#include "sloked/core/awaitable/Awaitable.h"
#include <mutex>
#include <map>

namespace sloked {

    class SlokedPosixAwaitable : public SlokedIOAwaitable {
     public:
        SlokedPosixAwaitable(int);
        int GetSocket() const;
        SystemId GetSystemId() const final;

        static const intptr_t PosixIOSystemId;
     private:
        int socket;
    };

    class SlokedPosixAwaitablePoll : public SlokedIOPoll {
     public:
        SlokedIOAwaitable::SystemId GetSystemId() const final;
        std::function<void()> Attach(std::unique_ptr<SlokedIOAwaitable>, std::function<void()>) final;
        void Await(std::chrono::system_clock::duration = std::chrono::system_clock::duration::zero()) final;

     private:
        std::mutex mtx;
        std::map<int, std::function<void()>> sockets;
    };
}

#endif