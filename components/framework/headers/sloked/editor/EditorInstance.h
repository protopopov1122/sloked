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

#ifndef SLOKED_EDITOR_EDITORINSTANCE_H_
#define SLOKED_EDITOR_EDITORINSTANCE_H_

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
#include "sloked/sched/Scheduler.h"
#include "sloked/sched/ThreadManager.h"
#include "sloked/services/CharPreset.h"

namespace sloked {

    class SlokedSharedEditorState : public SlokedCloseable {
     public:
        SlokedSharedEditorState(std::unique_ptr<SlokedIOPoll>);

        void Start();
        void Close() final;

        SlokedIOPoller &GetIO();
        SlokedSchedulerThread &GetScheduler();
        SlokedActionQueue &GetThreadManager();

     private:
        std::unique_ptr<SlokedIOPoll> ioPoll;
        SlokedSingleThreadActionQueue executor;
        SlokedDefaultIOPollThread ioPoller;
        SlokedDefaultScheduler scheduler;
        SlokedDefaultThreadManager threadManager;
    };

    class SlokedEditorInstance : public SlokedCloseable {
     public:
        SlokedEditorInstance(SlokedSharedEditorState &, SlokedSocketFactory &);
        SlokedCryptoFacade &InitializeCrypto(SlokedCrypto &);
        SlokedServerFacade &InitializeServer();
        SlokedServerFacade &InitializeServer(std::unique_ptr<SlokedSocket>);
        SlokedServiceDependencyProvider &InitializeServiceProvider(
            std::unique_ptr<SlokedServiceDependencyProvider>);
        SlokedScreenServer &InitializeScreen(SlokedScreenProviderFactory &,
                                             const SlokedUri &);
        void Attach(SlokedCloseable &);
        void Attach(std::unique_ptr<SlokedDataHandle>);

        bool IsRunning() const;
        void Start();
        void Stop();
        void Wait();
        void Close() final;

        SlokedCharPreset &GetCharPreset();
        SlokedSchedulerThread &GetScheduler();
        SlokedActionQueue &GetThreadManager();
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
        SlokedSharedEditorState &sharedState;
        std::atomic<bool> running;
        std::mutex termination_mtx;
        std::condition_variable termination_cv;
        SlokedCloseablePool closeables;
        SlokedNetworkFacade network;
        std::unique_ptr<SlokedCryptoFacade> crypto;
        std::unique_ptr<SlokedServerFacade> server;
        std::unique_ptr<SlokedCharPresetClient> charPresetUpdater;
        std::unique_ptr<SlokedServiceDependencyProvider> serviceProvider;
        std::unique_ptr<SlokedScreenProvider> screenProvider;
        std::unique_ptr<SlokedScreenServer> screen;
        SlokedCharPreset charPreset;
        std::vector<std::unique_ptr<SlokedDataHandle>> handles;
        KgrRunnableContextManagerHandle<KgrLocalContext> contextManager;
    };

    class SlokedEditorInstanceContainer {
     public:
        virtual ~SlokedEditorInstanceContainer() = default;
        virtual bool Has(const std::string &) const = 0;
        virtual SlokedEditorInstance &Get(const std::string &) const = 0;
        virtual void Enumerate(
            std::function<void(const std::string, SlokedEditorInstance &)>)
            const = 0;
        virtual SlokedEditorInstance &Spawn(const std::string &,
                                            const KgrValue &) = 0;
        virtual void Shutdown(const std::string &) = 0;
    };
}  // namespace sloked

#endif