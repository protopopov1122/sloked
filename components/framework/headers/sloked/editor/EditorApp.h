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
#include "sloked/core/DataHandle.h"
#include <atomic>
#include <variant>
#include <mutex>
#include <condition_variable>

namespace sloked {

    class SlokedEditorApp : public SlokedCloseable {
     public:
        SlokedEditorApp(std::unique_ptr<SlokedIOPoll>, SlokedSocketFactory &);
        SlokedCryptoFacade &InitializeCrypto(SlokedCrypto &);
        SlokedServerFacade &InitializeServer();
        SlokedServerFacade &InitializeServer(std::unique_ptr<SlokedSocket>);
        SlokedServiceDependencyProvider &InitializeServiceProvider(std::unique_ptr<SlokedServiceDependencyProvider>);
        void Attach(SlokedCloseable &);
        void Attach(std::unique_ptr<SlokedDataHandle>);

        bool IsRunning() const;
        void Start();
        void Stop();
        void Wait();
        void Close() final;

        SlokedCharWidth &GetCharWidth();
        SlokedSchedulerThread &GetScheduler();
        SlokedIOPoller &GetIO();
        SlokedNetworkFacade &GetNetwork();
        bool HasCrypto() const;
        SlokedCryptoFacade &GetCrypto();
        SlokedServerFacade &GetServer();
        SlokedServiceDependencyProvider &GetServiceProvider();

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
        std::unique_ptr<SlokedServiceDependencyProvider> serviceProvider;
        SlokedCharWidth charWidth;
        std::vector<std::unique_ptr<SlokedDataHandle>> handles;
    };

    class SlokedEditorAppContainer {
     public:
        virtual ~SlokedEditorAppContainer() = default;
        virtual bool Has(const std::string &) const = 0;
        virtual SlokedEditorApp &Get(const std::string &) const = 0;
        virtual void Enumerate(std::function<void(const std::string, SlokedEditorApp &)>) const = 0;
        virtual SlokedEditorApp &Spawn(const std::string &, const KgrValue &) = 0;
        virtual void Shutdown(const std::string &) = 0;
    };
}

#endif