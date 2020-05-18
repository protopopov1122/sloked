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

#ifndef SLOKED_CORE_AWAITABLE_WIN32_H_
#define SLOKED_CORE_AWAITABLE_WIN32_H_

#include <atomic>
#include <map>
#include <mutex>
#include <Winsock2.h>

#include "sloked/core/awaitable/Awaitable.h"

namespace sloked {

    class SlokedWin32Awaitable : public SlokedIOAwaitable {
     public:
        SlokedWin32Awaitable(int);
        int GetSocket() const;
        SystemId GetSystemId() const final;

        static const intptr_t Win32IOSystemId;

     private:
        int socket;
    };

    class SlokedWin32AwaitablePoll : public SlokedIOPoll {
     public:
        SlokedWin32AwaitablePoll();
        SlokedIOAwaitable::SystemId GetSystemId() const final;
        bool Empty() const final;
        std::function<void()> Attach(std::unique_ptr<SlokedIOAwaitable>,
                                     std::function<void()>) final;
        void Await(std::chrono::system_clock::duration =
                       std::chrono::system_clock::duration::zero()) final;

     private:
        std::mutex mtx;
        std::atomic<int> max_socket;
        std::atomic<fd_set> descriptors;
        std::map<int, std::function<void()>> sockets;
        std::vector<std::function<void()>> callbacks;
    };
}  // namespace sloked

#endif