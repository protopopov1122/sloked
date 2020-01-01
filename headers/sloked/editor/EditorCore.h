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

#ifndef SLOKED_EDITOR_EDITORCORE_H_
#define SLOKED_EDITOR_EDITORCORE_H_

#include "sloked/core/Closeable.h"
#include "sloked/core/Semaphore.h"
#include "sloked/core/Logger.h"
#include "sloked/core/CharWidth.h"
#include "sloked/core/awaitable/Poll.h"
#include "sloked/sched/Scheduler.h"
#include "sloked/kgr/local/Server.h"
#include "sloked/kgr/local/NamedServer.h"
#include "sloked/namespace/Object.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"
#include "sloked/kgr/ctx-manager/RunnableContextManager.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/net/Socket.h"
#include "sloked/kgr/net/MasterServer.h"
#include "sloked/editor/EditorServer.h"

namespace sloked {

    class SlokedAbstractEditorCore : public SlokedCloseable {
     public:
        ~SlokedAbstractEditorCore();
        SlokedLogger &GetLogger();
        KgrContextManager<KgrLocalContext> &GetContextManager();
        SlokedSchedulerThread &GetScheduler();
        SlokedIOPoller &GetIO();
        KgrNamedServer &GetServer();
        SlokedNamedRestrictionTarget &GetRestrictions();
        void Start();
        void SpawnNetServer(SlokedSocketFactory &, const std::string &, uint16_t, SlokedNamedRestrictionAuthority * = nullptr, SlokedAuthenticatorFactory * = nullptr);
        void Close() final;

     protected:
        SlokedAbstractEditorCore(SlokedLogger &, SlokedIOPoller &);

        SlokedCloseablePool closeables;
        KgrRunnableContextManagerHandle<KgrLocalContext> contextManager;
        SlokedLogger &logger;
        SlokedDefaultSchedulerThread sched;
        SlokedIOPoller &io;
        std::unique_ptr<SlokedEditorServer> server;
        std::unique_ptr<KgrMasterNetServer> netServer;
    };

    class SlokedEditorMasterCore : public SlokedAbstractEditorCore {
     public:
        SlokedEditorMasterCore(SlokedLogger &, SlokedIOPoller &, SlokedNamespace &, const SlokedCharWidth &);
        SlokedEditorMasterCore(std::unique_ptr<SlokedSocket>, SlokedLogger &, SlokedIOPoller &, SlokedAuthenticatorFactory *, SlokedNamespace &, const SlokedCharWidth &);
        SlokedTextTaggerRegistry<int> &GetTaggers();

     private:
        void Init(SlokedNamespace &, const SlokedCharWidth &);
        SlokedEditorDocumentSet documents;
        SlokedTextTaggerRegistry<int> taggers;
    };

    class SlokedEditorSlaveCore : public SlokedAbstractEditorCore {
     public:
        SlokedEditorSlaveCore(std::unique_ptr<SlokedSocket>, SlokedLogger &, SlokedIOPoller &, SlokedAuthenticatorFactory *);
        void Authorize(const std::string &);

     private:
        SlokedRemoteEditorServer *remoteEditor;
    };
}

#endif