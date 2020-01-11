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
#include "sloked/core/awaitable/Poll.h"
#include "sloked/core/CharWidth.h"
#include "sloked/sched/Scheduler.h"
#include "sloked/facade/Crypto.h"
#include "sloked/facade/Network.h"
#include "sloked/facade/Server.h"
#include "sloked/facade/Services.h"
#include <atomic>
#include <variant>
#include <mutex>
#include <condition_variable>

namespace sloked {

    class SlokedEditorApp {
     public:
        SlokedEditorApp(std::unique_ptr<SlokedIOPoll>, SlokedSocketFactory &);
        SlokedCryptoFacade &InitializeCrypto(SlokedCrypto &);
        SlokedServerFacade &InitializeServer();
        SlokedServerFacade &InitializeServer(std::unique_ptr<SlokedSocket>);
        SlokedAbstractServicesFacade &InitializeServices(std::unique_ptr<SlokedAbstractServicesFacade>);
        void Attach(SlokedCloseable &);

        bool IsRunning() const;
        void Start();
        void Stop();
        void Wait();

        SlokedCharWidth &GetCharWidth();
        SlokedSchedulerThread &GetScheduler();
        SlokedIOPoller &GetIO();
        SlokedNetworkFacade &GetNetwork();
        SlokedCryptoFacade &GetCrypto();
        SlokedServerFacade &GetServer();

     private:
        std::atomic<bool> running;
        std::mutex termination_mtx;
        std::condition_variable termination_cv;
        SlokedCloseablePool closeables;
        SlokedDefaultSchedulerThread sched;
        std::unique_ptr<SlokedIOPoll> ioPoll;
        std::unique_ptr<SlokedDefaultIOPollThread> ioPoller;
        SlokedNetworkFacade network;
        std::unique_ptr<SlokedCryptoFacade> crypto;
        std::unique_ptr<SlokedServerFacade> server;
        std::unique_ptr<SlokedAbstractServicesFacade> services;
        SlokedCharWidth charWidth;
    };
}

#endif