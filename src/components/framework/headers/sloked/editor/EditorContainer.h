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

#ifndef SLOKED_EDITOR_EDITORCONTAINER_H_
#define SLOKED_EDITOR_EDITORCONTAINER_H_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <variant>

#include "sloked/core/CharPreset.h"
#include "sloked/core/Closeable.h"
#include "sloked/core/DataHandle.h"
#include "sloked/core/awaitable/Poll.h"
#include "sloked/editor/ScreenServer.h"
#include "sloked/facade/Crypto.h"
#include "sloked/facade/Network.h"
#include "sloked/facade/Server.h"
#include "sloked/facade/Services.h"
#include "sloked/sched/DefaultScheduler.h"
#include "sloked/sched/MultithreadExecutor.h"
#include "sloked/sched/SequentialExecutor.h"

namespace sloked {

    class SlokedSharedContainerEnvironment : public SlokedCloseable {
     public:
        SlokedSharedContainerEnvironment(std::unique_ptr<SlokedIOPoll>);

        void Start();
        void Close() final;

        SlokedIOPoller &GetIO();
        SlokedScheduler &GetScheduler();
        SlokedExecutor &GetExecutor();
        SlokedExecutor &GetThreadedExecutor();

     private:
        std::unique_ptr<SlokedIOPoll> ioPoll;
        SlokedSequentialExecutor executor;
        SlokedDefaultIOPollThread ioPoller;
        SlokedDefaultScheduler scheduler;
        SlokedMultitheadExecutor threadManager;
    };

    class SlokedEditorContainer : public SlokedCloseable {
     public:
        SlokedEditorContainer(SlokedSharedContainerEnvironment &, SlokedSocketFactory &);
        SlokedCryptoFacade &InitializeCrypto(SlokedCrypto &);
        SlokedServerFacade &InitializeServer();
        SlokedServerFacade &InitializeServer(std::unique_ptr<SlokedSocket>);
        SlokedServiceDependencyProvider &InitializeServiceProvider(
            std::unique_ptr<SlokedServiceDependencyProvider>);
        SlokedScreenServer &InitializeScreen(SlokedScreenProviderFactory &,
                                             const SlokedUri &, std::unique_ptr<SlokedCharPreset>);
        void Attach(SlokedCloseable &);
        void Attach(std::unique_ptr<SlokedDataHandle>);

        bool IsRunning() const;
        void Start();
        void Stop();
        void Wait();
        void Close() final;

        SlokedScheduler &GetScheduler();
        SlokedExecutor &GetExecutor();
        SlokedExecutor &GetThreadedExecutor();
        SlokedIOPoller &GetIO();
        SlokedNetworkFacade &GetNetwork();
        bool HasCrypto() const;
        SlokedCryptoFacade &GetCrypto();
        SlokedServerFacade &GetServer();
        SlokedServiceDependencyProvider &GetServiceProvider();
        bool HasScreen() const;
        SlokedScreenServer &GetScreen() const;
        KgrContextManager<KgrLocalContext> &GetContextManager();

     private:
        SlokedSharedContainerEnvironment &sharedState;
        std::atomic<bool> running;
        std::mutex termination_mtx;
        std::condition_variable termination_cv;
        SlokedCloseablePool closeables;
        SlokedNetworkFacade network;
        std::unique_ptr<SlokedCryptoFacade> crypto;
        std::unique_ptr<SlokedServerFacade> server;
        std::unique_ptr<SlokedServiceDependencyProvider> serviceProvider;
        std::unique_ptr<SlokedScreenProvider> screenProvider;
        std::unique_ptr<SlokedScreenServer> screen;
        std::vector<std::unique_ptr<SlokedDataHandle>> handles;
        KgrRunnableContextManagerHandle<KgrLocalContext> contextManager;
    };

    class SlokedEditorContainers {
     public:
        virtual ~SlokedEditorContainers() = default;
        virtual bool Has(const std::string &) const = 0;
        virtual SlokedEditorContainer &Get(const std::string &) const = 0;
        virtual void Enumerate(
            std::function<void(const std::string, SlokedEditorContainer &)>)
            const = 0;
        virtual SlokedEditorContainer &Spawn(const std::string &,
                                            const KgrValue &) = 0;
        virtual void Shutdown(const std::string &) = 0;
    };
}  // namespace sloked

#endif