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

#ifndef SLOKED_CORE_AWAITABLE_POLL_H_
#define SLOKED_CORE_AWAITABLE_POLL_H_

#include <atomic>
#include <map>
#include <mutex>
#include <thread>

#include "sloked/core/Closeable.h"
#include "sloked/core/Counter.h"
#include "sloked/core/Scope.h"
#include "sloked/core/awaitable/Awaitable.h"
#include "sloked/sched/Executor.h"

namespace sloked {

    class SlokedIOPoller {
     public:
        using Handle = OnDestroy;

        class Awaitable {
         public:
            virtual ~Awaitable() = default;
            virtual std::unique_ptr<SlokedIOAwaitable> GetAwaitable() const = 0;
            virtual void Process(bool) = 0;
        };

        virtual ~SlokedIOPoller() = default;
        virtual Handle Attach(std::unique_ptr<Awaitable>) = 0;
    };

    class SlokedDefaultIOPollThread : public SlokedIOPoller,
                                      public SlokedCloseable {
     public:
        SlokedDefaultIOPollThread(SlokedIOPoll &, SlokedExecutor &);
        ~SlokedDefaultIOPollThread();
        void Start(std::chrono::system_clock::duration);
        void Close() final;
        Handle Attach(std::unique_ptr<Awaitable>) final;

     private:
        SlokedIOPoll &poll;
        SlokedExecutor &executor;
        std::thread worker;
        std::atomic<bool> work;
        std::map<std::size_t, std::shared_ptr<Awaitable>> awaitables;
        std::size_t nextId;
        std::mutex mtx;
        std::condition_variable cv;
    };
}  // namespace sloked

#endif