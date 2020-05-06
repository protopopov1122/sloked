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

#ifndef SLOKED_FACADE_SERVICES_H_
#define SLOKED_FACADE_SERVICES_H_

#include "sloked/core/CharPreset.h"
#include "sloked/core/Logger.h"
#include "sloked/editor/Shutdown.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"
#include "sloked/kgr/NamedServer.h"
#include "sloked/kgr/ctx-manager/RunnableContextManager.h"
#include "sloked/kgr/local/Context.h"
#include "sloked/namespace/Mount.h"
#include "sloked/namespace/Root.h"
#include "sloked/security/Master.h"
#include "sloked/text/fragment/TaggedText.h"

namespace sloked {

    class SlokedServiceDependencyProvider : public SlokedCloseable {
     public:
        virtual ~SlokedServiceDependencyProvider() = default;
        virtual KgrContextManager<KgrLocalContext> &GetContextManager() = 0;
        virtual SlokedTextTaggerRegistry<SlokedEditorDocument::TagType>
            &GetTaggers() = 0;
        virtual SlokedLogger &GetLogger() = 0;
        virtual SlokedRootNamespace &GetNamespace() = 0;
        virtual KgrNamedServer &GetServer() = 0;
        virtual SlokedEditorDocumentSet &GetDocuments() = 0;
        virtual SlokedCredentialMaster *GetCredentialMaster() = 0;
        virtual SlokedEditorShutdown &GetShutdown() = 0;
    };

    class SlokedServiceDependencyDefaultProvider
        : public SlokedServiceDependencyProvider {
     public:
        SlokedServiceDependencyDefaultProvider(
            SlokedLogger &, std::unique_ptr<SlokedRootNamespace>,
            KgrNamedServer &, KgrContextManager<KgrLocalContext> &,
            SlokedCredentialMaster *, SlokedEditorShutdown &,
            SlokedTextTaggerRegistry<SlokedEditorDocument::TagType> * =
                nullptr);
        KgrContextManager<KgrLocalContext> &GetContextManager() override;
        SlokedTextTaggerRegistry<SlokedEditorDocument::TagType> &GetTaggers()
            override;
        SlokedLogger &GetLogger() override;
        SlokedRootNamespace &GetNamespace() override;
        KgrNamedServer &GetServer() override;
        SlokedEditorDocumentSet &GetDocuments() override;
        SlokedCredentialMaster *GetCredentialMaster() override;
        SlokedEditorShutdown &GetShutdown() override;
        void Close() override;

     protected:
        SlokedLogger &logger;
        std::unique_ptr<SlokedRootNamespace> rootNamespace;
        KgrNamedServer &server;
        SlokedEditorDocumentSet documents;
        KgrContextManager<KgrLocalContext> &contextManager;
        SlokedCredentialMaster *credentialMaster;
        SlokedEditorShutdown &shutdown;
        SlokedDefaultTextTaggerRegistry<SlokedEditorDocument::TagType> taggers;
    };

    class SlokedAbstractServicesFacade {
     public:
        SlokedAbstractServicesFacade(SlokedServiceDependencyProvider &);
        virtual ~SlokedAbstractServicesFacade() = default;
        SlokedServiceDependencyProvider &GetProvider() const;
        virtual std::unique_ptr<KgrService> Build(const std::string &) = 0;

     protected:
        SlokedServiceDependencyProvider &provider;
    };

    class SlokedDefaultServicesFacade : public SlokedAbstractServicesFacade {
     public:
        SlokedDefaultServicesFacade(SlokedServiceDependencyProvider &);
        std::unique_ptr<KgrService> Build(const std::string &) final;

     private:
        std::map<SlokedPath, std::function<std::unique_ptr<KgrService>(
                                 SlokedServiceDependencyProvider &)>>
            builders;
    };
}  // namespace sloked

#endif