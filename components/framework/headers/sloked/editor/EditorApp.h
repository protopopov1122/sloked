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

#ifndef SLOKED_EDITOR_EDITORAPP_H_
#define SLOKED_EDITOR_EDITORAPP_H_

#include "sloked/core/Closeable.h"
#include "sloked/core/Semaphore.h"
#include "sloked/core/awaitable/Poll.h"
#include "sloked/core/CharWidth.h"
#include "sloked/sched/Scheduler.h"
#include "sloked/net/Socket.h"
#include "sloked/core/Crypto.h"
#include <atomic>

namespace sloked {

    class SlokedEditorApp {
     public:
        SlokedEditorApp();

        bool IsRunning() const;
        void Initialize(std::unique_ptr<SlokedIOPoll>, SlokedSocketFactory &);
        void InitializeCrypto(SlokedCrypto &);
        void RequestStop();
        void WaitForStop();
        void Attach(SlokedCloseable &);

        SlokedCharWidth &GetCharWidth();
        SlokedSchedulerThread &GetScheduler();
        SlokedIOPoller &GetIO();
        SlokedSocketFactory &GetNetwork();
        SlokedCrypto &GetCrypto();

     private:
        std::atomic<bool> running;
        SlokedCloseablePool closeables;
        SlokedSemaphore termination;
        SlokedDefaultSchedulerThread sched;
        std::unique_ptr<SlokedIOPoll> ioPoll;
        std::unique_ptr<SlokedDefaultIOPollThread> ioPoller;
        SlokedSocketFactory *network;
        SlokedCrypto *crypto;
        SlokedCharWidth charWidth;
    };
}

#endif