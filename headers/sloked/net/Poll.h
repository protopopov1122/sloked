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

#ifndef SLOKED_NET_POLL_H_
#define SLOKED_NET_POLL_H_

#include "sloked/net/Socket.h"
#include "sloked/core/Counter.h"
#include "sloked/core/Scope.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <map>

namespace sloked {

    class SlokedSocketPoller {
     public:
        using Handle = OnDestroy;

        class Awaitable {
         public:
            virtual ~Awaitable() = default;
            virtual std::unique_ptr<SlokedSocketAwaitable> GetAwaitable() const = 0;
            virtual void Process(bool) = 0;
        };

        virtual ~SlokedSocketPoller() = default;
        virtual Handle Attach(std::unique_ptr<Awaitable>) = 0;
    };

    class SlokedDefaultSocketPollThread : public SlokedSocketPoller {
     public:
        SlokedDefaultSocketPollThread(SlokedSocketPoll &);
        void Start(long);
        void Stop();
        Handle Attach(std::unique_ptr<Awaitable>) final;

     private:
        SlokedSocketPoll &poll;
        std::thread worker;
        std::atomic<bool> work;
        std::map<std::size_t, std::unique_ptr<Awaitable>> awaitables;
        std::size_t nextId;
        std::mutex queueMtx;
        std::map<std::size_t, std::unique_ptr<Awaitable>> awaitableQueue;
        std::vector<std::size_t> removalQueue;
    };
}

#endif