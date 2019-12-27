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

    SlokedAbstractEditorCore::~SlokedAbstractEditorCore() {
        this->Close();
    }

    SlokedLogger &SlokedAbstractEditorCore::GetLogger() {
        return this->logger;
    }

    KgrContextManager<KgrLocalContext> &SlokedAbstractEditorCore::GetContextManager() {
        return this->contextManager.GetManager();
    }

    SlokedSchedulerThread &SlokedAbstractEditorCore::GetScheduler() {
        return this->sched;
    }
    SlokedIOPoller &SlokedAbstractEditorCore::GetIO() {
        return this->io;
    }

    KgrNamedServer &SlokedAbstractEditorCore::GetServer() {
        if (this->server != nullptr) {
            return this->server->GetServer();
        } else {
            throw SlokedError("SlokedEditorCore: Server not defined");
        }
    }

    KgrNamedRestrictionManager &SlokedAbstractEditorCore::GetRestrictions() {
        if (this->server != nullptr) {
            return this->server->GetRestrictions();
        } else {
            throw SlokedError("SlokedEditorCore: Server not defined");
        }
    }

    KgrNamedRestrictionManager &SlokedAbstractEditorCore::GetNetRestrictions() {
        if (this->netServer != nullptr) {
            return this->netServer->GetRestrictions();
        } else {
            throw SlokedError("SlokedEditorCore: NetServer not defined");
        }
    }

    void SlokedAbstractEditorCore::Start() {
        this->closeables.Attach(this->contextManager);
        this->contextManager.Start();
        this->closeables.Attach(this->sched);
        this->sched.Start();
        if (this->server != nullptr) {
            this->closeables.Attach(*this->server);
            this->server->Start();
        } else {
            throw SlokedError("SlokedEditorCore: Server not defined");
        }
    }

    void SlokedAbstractEditorCore::Close() {
        this->closeables.Close();
    }

    SlokedAbstractEditorCore::SlokedAbstractEditorCore(SlokedLogger &logger, SlokedIOPoller &io)
        : logger(logger), io(io), server(nullptr) {}
    
    SlokedEditorMasterCore::SlokedEditorMasterCore(SlokedLogger &logger, SlokedIOPoller &io, SlokedNamespace &root, const SlokedCharWidth &charWidth)
        : SlokedAbstractEditorCore(logger, io), documents(root) {
        this->server = std::make_unique<SlokedLocalEditorServer>();
        this->Init(root, charWidth);
    }
    
    SlokedEditorMasterCore::SlokedEditorMasterCore(std::unique_ptr<SlokedSocket> socket, SlokedLogger &logger, SlokedIOPoller &io, SlokedNamespace &root, const SlokedCharWidth &charWidth)
        : SlokedAbstractEditorCore(logger, io), documents(root) {
        this->server = std::make_unique<SlokedRemoteEditorServer>(std::move(socket), this->io);
        this->Init(root, charWidth);
    }

    SlokedTextTaggerRegistry<int> &SlokedEditorMasterCore::GetTaggers() {
        return this->taggers;
    }

    void SlokedEditorMasterCore::Init(SlokedNamespace &root, const SlokedCharWidth &charWidth) {
        this->GetServer().Register("document::render", std::make_unique<SlokedTextRenderService>(this->documents, charWidth, this->taggers, this->contextManager.GetManager()));
        this->GetServer().Register("document::cursor", std::make_unique<SlokedCursorService>(this->documents, this->GetServer().GetConnector("document::render"), this->contextManager.GetManager()));
        this->GetServer().Register("document::manager", std::make_unique<SlokedDocumentSetService>(this->documents, this->contextManager.GetManager()));
        this->GetServer().Register("document::notify", std::make_unique<SlokedDocumentNotifyService>(this->documents, this->contextManager.GetManager()));
        this->GetServer().Register("document::search", std::make_unique<SlokedSearchService>(this->documents, this->contextManager.GetManager()));
        this->GetServer().Register("namespace::root", std::make_unique<SlokedNamespaceService>(root, this->contextManager.GetManager()));
    }

    SlokedEditorSlaveCore::SlokedEditorSlaveCore(std::unique_ptr<SlokedSocket> socket, SlokedLogger &logger, SlokedIOPoller &io)
        : SlokedAbstractEditorCore(logger, io) {
        this->server = std::make_unique<SlokedRemoteEditorServer>(std::move(socket), this->io);
    }

    void SlokedAbstractEditorCore::SpawnNetServer(SlokedSocketFactory &socketFactory, const std::string &host, uint16_t port, const SlokedAuthenticator *auth) {
        if (this->netServer == nullptr) {
            this->netServer = std::make_unique<KgrMasterNetServer>(this->server->GetServer(), socketFactory.Bind(host, port), this->io, auth);
            this->closeables.Attach(*this->netServer);
            this->netServer->Start();
        } else {
            throw SlokedError("Editor: network server already spawned");
        }
    }
}