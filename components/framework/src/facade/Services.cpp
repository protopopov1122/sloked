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

    SlokedServiceDependencyDefaultProvider::SlokedServiceDependencyDefaultProvider(SlokedLogger &logger, SlokedMountableNamespace &root, SlokedNamespaceMounter &mounter, const SlokedCharWidth &charWidth, KgrNamedServer &server)
        : logger(logger), root(root), mounter(mounter), charWidth(charWidth), server(server), documents(root) {}

    KgrContextManager<KgrLocalContext> &SlokedServiceDependencyDefaultProvider::GetContextManager() {
        return this->contextManager.GetManager();
    }

    SlokedTextTaggerRegistry<int> &SlokedServiceDependencyDefaultProvider::GetTaggers() {
        return this->taggers;
    }

    SlokedLogger &SlokedServiceDependencyDefaultProvider::GetLogger() {
        return this->logger;
    }
    
    SlokedMountableNamespace &SlokedServiceDependencyDefaultProvider::GetRoot() {
        return this->root;
    }

    SlokedNamespaceMounter &SlokedServiceDependencyDefaultProvider::GetMounter() {
        return this->mounter;
    }

    const SlokedCharWidth &SlokedServiceDependencyDefaultProvider::GetCharWidth() {
        return this->charWidth;
    }

    KgrNamedServer &SlokedServiceDependencyDefaultProvider::GetServer() {
        return this->server;
    }

    SlokedEditorDocumentSet &SlokedServiceDependencyDefaultProvider::GetDocuments() {
        return this->documents;
    }

    void SlokedServiceDependencyDefaultProvider::Close() {
        this->contextManager.Close();
    }

    void SlokedServiceDependencyDefaultProvider::Start() {
        this->contextManager.Start();
    }

    SlokedAbstractServicesFacade::SlokedAbstractServicesFacade(SlokedServiceDependencyProvider &provider)
        : provider(provider) {}

    SlokedServiceDependencyProvider &SlokedAbstractServicesFacade::GetProvider() const {
        return this->provider;
    }

    SlokedDefaultServicesFacade::SlokedDefaultServicesFacade(SlokedServiceDependencyProvider &provider)
        : SlokedAbstractServicesFacade(provider) {
        
        this->builders.emplace("document::render", [](SlokedServiceDependencyProvider &provider) {
            return std::make_unique<SlokedTextRenderService>(provider.GetDocuments(), provider.GetCharWidth(), provider.GetTaggers(), provider.GetContextManager());
        });
        this->builders.emplace("document::cursor", [](SlokedServiceDependencyProvider &provider) {
            return std::make_unique<SlokedCursorService>(provider.GetDocuments(), provider.GetServer().GetConnector("document::render"), provider.GetContextManager());
        });
        this->builders.emplace("document::manager", [](SlokedServiceDependencyProvider &provider) {
            return std::make_unique<SlokedDocumentSetService>(provider.GetDocuments(), provider.GetContextManager());
        });
        this->builders.emplace("document::notify", [](SlokedServiceDependencyProvider &provider) {
            return std::make_unique<SlokedDocumentNotifyService>(provider.GetDocuments(), provider.GetContextManager());
        });
        this->builders.emplace("document::search", [](SlokedServiceDependencyProvider &provider) {
            return std::make_unique<SlokedSearchService>(provider.GetDocuments(), provider.GetContextManager());
        });
        this->builders.emplace("namespace::root", [](SlokedServiceDependencyProvider &provider) {
            return std::make_unique<SlokedNamespaceService>(provider.GetRoot(), provider.GetMounter(), provider.GetContextManager());
        });
    }

    std::unique_ptr<KgrService> SlokedDefaultServicesFacade::Build(const std::string &id) {
        if (this->builders.count(id) != 0) {
            return this->builders.at(id)(this->provider);
        } else {
            return nullptr;
        }
    }
}