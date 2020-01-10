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
#include "sloked/facade/Crypto.h"
#include "sloked/facade/Network.h"
#include "sloked/facade/Server.h"
#include <atomic>
#include <variant>

namespace sloked {

    class SlokedEditorApp {
     public:
        SlokedEditorApp(std::unique_ptr<SlokedIOPoll>, SlokedSocketFactory &);
        SlokedCryptoFacade &InitializeCrypto(SlokedCrypto &);
        SlokedServerFacade &InitializeServer();
        SlokedServerFacade &InitializeServer(std::unique_ptr<SlokedSocket>, SlokedIOPoller &, SlokedAuthenticatorFactory *);
        void RequestStop();
        void WaitForStop();
        void Attach(SlokedCloseable &);

        SlokedCharWidth &GetCharWidth();
        SlokedSchedulerThread &GetScheduler();
        SlokedIOPoller &GetIO();
        SlokedNetworkFacade &GetNetwork();
        SlokedCryptoFacade &GetCrypto();
        SlokedServerFacade &GetServer();

     private:
        SlokedCloseablePool closeables;
        SlokedSemaphore termination;
        SlokedDefaultSchedulerThread sched;
        std::unique_ptr<SlokedIOPoll> ioPoll;
        std::unique_ptr<SlokedDefaultIOPollThread> ioPoller;
        std::unique_ptr<SlokedNetworkFacade> network;
        std::unique_ptr<SlokedCryptoFacade> crypto;
        std::unique_ptr<SlokedServerFacade> server;
        SlokedCharWidth charWidth;
    };
}

#endif