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

#include "sloked/facade/Services.h"
#include "sloked/services/Cursor.h"
#include "sloked/services/DocumentNotify.h"
#include "sloked/services/DocumentSet.h"
#include "sloked/services/Namespace.h"
#include "sloked/services/Search.h"
#include "sloked/services/TextRender.h"

namespace sloked {

    SlokedAbstractServicesFacade::SlokedAbstractServicesFacade(SlokedLogger &logger, SlokedMountableNamespace &root, SlokedNamespaceMounter &mounter, const SlokedCharWidth &charWidth)
        : logger(logger), root(root), mounter(mounter), charWidth(charWidth), documents(root) {}

    KgrContextManager<KgrLocalContext> &SlokedAbstractServicesFacade::GetContextManager() {
        return this->contextManager.GetManager();
    }

    SlokedTextTaggerRegistry<int> &SlokedAbstractServicesFacade::GetTaggers() {
        return this->taggers;
    }

    void SlokedAbstractServicesFacade::Start() {
        this->contextManager.Start();
    }

    void SlokedAbstractServicesFacade::Close() {
        this->contextManager.Close();
    }

    void SlokedDefaultServicesFacade::Apply(KgrNamedServer &server) {
        server.Register("document::render", std::make_unique<SlokedTextRenderService>(this->documents, this->charWidth, this->taggers, this->contextManager.GetManager()));
        server.Register("document::cursor", std::make_unique<SlokedCursorService>(this->documents, server.GetConnector("document::render"), this->contextManager.GetManager()));
        server.Register("document::manager", std::make_unique<SlokedDocumentSetService>(this->documents, this->contextManager.GetManager()));
        server.Register("document::notify", std::make_unique<SlokedDocumentNotifyService>(this->documents, this->contextManager.GetManager()));
        server.Register("document::search", std::make_unique<SlokedSearchService>(this->documents, this->contextManager.GetManager()));
        server.Register("namespace::root", std::make_unique<SlokedNamespaceService>(root, mounter, this->contextManager.GetManager()));
    }
}