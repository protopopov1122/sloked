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

#include "sloked/editor/EditorCore.h"
#include "sloked/services/Cursor.h"
#include "sloked/services/DocumentNotify.h"
#include "sloked/services/DocumentSet.h"
#include "sloked/services/Namespace.h"
#include "sloked/services/Search.h"
#include "sloked/services/TextRender.h"
#include "sloked/kgr/net/Config.h"

namespace sloked {

    SlokedEditorCore::SlokedEditorCore(SlokedLogger &logger, SlokedIOPoll &io, SlokedNamespace &root, const SlokedCharWidth &charWidth)
        : logger(logger), io(io), server(rawServer), documents(root), netServer(nullptr) {
        
        this->server.Register("document::render", std::make_unique<SlokedTextRenderService>(this->documents, charWidth, this->taggers, this->contextManager.GetManager()));
        this->server.Register("document::cursor", std::make_unique<SlokedCursorService>(this->documents, this->server.GetConnector("document::render"), this->contextManager.GetManager()));
        this->server.Register("document::manager", std::make_unique<SlokedDocumentSetService>(this->documents, this->contextManager.GetManager()));
        this->server.Register("document::notify", std::make_unique<SlokedDocumentNotifyService>(this->documents, this->contextManager.GetManager()));
        this->server.Register("document::search", std::make_unique<SlokedSearchService>(this->documents, this->contextManager.GetManager()));
        this->server.Register("namespace::root", std::make_unique<SlokedNamespaceService>(root, this->contextManager.GetManager()));
    }

    KgrNamedServer &SlokedEditorCore::GetServer() {
        return this->server;
    }

    SlokedTextTaggerRegistry<int> &SlokedEditorCore::GetTaggers() {
        return this->taggers;
    }

    KgrContextManager<KgrLocalContext> &SlokedEditorCore::GetContextManager() {
        return this->contextManager.GetManager();
    }

    SlokedSchedulerThread &SlokedEditorCore::GetScheduler() {
        return this->sched;
    }

    SlokedIOPoller &SlokedEditorCore::GetIO() {
        return this->io;
    }

    void SlokedEditorCore::SpawnNetServer(SlokedSocketFactory &socketFactory, const std::string &host, uint16_t port) {
        if (this->netServer == nullptr) {
            this->netServer = std::make_unique<KgrMasterNetServer>(this->server, socketFactory.Bind(host, port), this->io);
            this->closeables.Attach(*this->netServer);
            this->netServer->Start();
        } else {
            throw SlokedError("Editor: network server already spawned");
        }
    }

    void SlokedEditorCore::Start() {
        this->closeables.Attach(this->contextManager);
        this->contextManager.Start();
        this->closeables.Attach(this->sched);
        this->closeables.Attach(this->io);
        this->io.Start(KgrNetConfig::RequestTimeout);
        this->sched.Start();
    }

    void SlokedEditorCore::Close() {
        this->closeables.Close();
    }
}