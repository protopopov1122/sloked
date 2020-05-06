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

#include "sloked/facade/Services.h"

#include "sloked/services/CredentialMaster.h"
#include "sloked/services/Cursor.h"
#include "sloked/services/DocumentNotify.h"
#include "sloked/services/DocumentSet.h"
#include "sloked/services/Namespace.h"
#include "sloked/services/Search.h"
#include "sloked/services/Shutdown.h"
#include "sloked/services/TextRender.h"

namespace sloked {

    SlokedServiceDependencyDefaultProvider::
        SlokedServiceDependencyDefaultProvider(
            SlokedLogger &logger,
            std::unique_ptr<SlokedRootNamespace> rootNamespace,
            KgrNamedServer &server,
            KgrContextManager<KgrLocalContext> &contextManager,
            SlokedCredentialMaster *credentialMaster,
            SlokedEditorShutdown &shutdown,
            SlokedTextTaggerRegistry<SlokedEditorDocument::TagType>
                *baseTaggers)
        : logger(logger), rootNamespace(std::move(rootNamespace)),
          server(server), documents(this->rootNamespace->GetRoot()),
          contextManager(contextManager), credentialMaster(credentialMaster),
          shutdown(shutdown), taggers(baseTaggers) {}

    KgrContextManager<KgrLocalContext>
        &SlokedServiceDependencyDefaultProvider::GetContextManager() {
        return this->contextManager;
    }

    SlokedTextTaggerRegistry<SlokedEditorDocument::TagType>
        &SlokedServiceDependencyDefaultProvider::GetTaggers() {
        return this->taggers;
    }

    SlokedLogger &SlokedServiceDependencyDefaultProvider::GetLogger() {
        return this->logger;
    }

    SlokedRootNamespace &
        SlokedServiceDependencyDefaultProvider::GetNamespace() {
        return *this->rootNamespace;
    }

    KgrNamedServer &SlokedServiceDependencyDefaultProvider::GetServer() {
        return this->server;
    }

    SlokedEditorDocumentSet &
        SlokedServiceDependencyDefaultProvider::GetDocuments() {
        return this->documents;
    }

    SlokedCredentialMaster *
        SlokedServiceDependencyDefaultProvider::GetCredentialMaster() {
        return this->credentialMaster;
    }

    SlokedEditorShutdown &
        SlokedServiceDependencyDefaultProvider::GetShutdown() {
        return this->shutdown;
    }

    void SlokedServiceDependencyDefaultProvider::Close() {}

    SlokedAbstractServicesFacade::SlokedAbstractServicesFacade(
        SlokedServiceDependencyProvider &provider)
        : provider(provider) {}

    SlokedServiceDependencyProvider &SlokedAbstractServicesFacade::GetProvider()
        const {
        return this->provider;
    }

    SlokedDefaultServicesFacade::SlokedDefaultServicesFacade(
        SlokedServiceDependencyProvider &provider)
        : SlokedAbstractServicesFacade(provider) {

        this->builders.emplace(
            "/document/render", [](SlokedServiceDependencyProvider &provider) {
                return std::make_unique<SlokedTextRenderService>(
                    provider.GetDocuments(), provider.GetContextManager());
            });
        this->builders.emplace(
            "/document/cursor", [](SlokedServiceDependencyProvider &provider) {
                return std::make_unique<SlokedCursorService>(
                    provider.GetDocuments(),
                    provider.GetServer().GetConnector({"/document/render"}),
                    provider.GetContextManager());
            });
        this->builders.emplace(
            "/document/manager", [](SlokedServiceDependencyProvider &provider) {
                return std::make_unique<SlokedDocumentSetService>(
                    provider.GetDocuments(), provider.GetTaggers(),
                    provider.GetContextManager());
            });
        this->builders.emplace(
            "/document/notify", [](SlokedServiceDependencyProvider &provider) {
                return std::make_unique<SlokedDocumentNotifyService>(
                    provider.GetDocuments(), provider.GetContextManager());
            });
        this->builders.emplace(
            "/document/search", [](SlokedServiceDependencyProvider &provider) {
                return std::make_unique<SlokedSearchService>(
                    provider.GetDocuments(), provider.GetContextManager());
            });
        this->builders.emplace(
            "/namespace/root", [](SlokedServiceDependencyProvider &provider) {
                return std::make_unique<SlokedNamespaceService>(
                    provider.GetNamespace(), provider.GetContextManager());
            });
        this->builders.emplace(
            "/editor/authorization",
            [](SlokedServiceDependencyProvider &provider)
                -> std::unique_ptr<KgrService> {
                if (provider.GetCredentialMaster()) {
                    return std::make_unique<SlokedCredentialMasterService>(
                        *provider.GetCredentialMaster(),
                        provider.GetContextManager());
                } else {
                    return nullptr;
                }
            });
        this->builders.emplace(
            "/editor/shutdown", [](SlokedServiceDependencyProvider &provider) {
                return std::make_unique<SlokedShutdownService>(
                    provider.GetShutdown(), provider.GetContextManager());
            });
    }

    std::unique_ptr<KgrService> SlokedDefaultServicesFacade::Build(
        const std::string &id) {
        SlokedPath path{id};
        if (!path.IsAbsolute()) {
            path = path.RelativeTo(path.Root());
        }
        if (this->builders.count(path) != 0) {
            return this->builders.at(path)(this->provider);
        } else {
            return nullptr;
        }
    }
}  // namespace sloked