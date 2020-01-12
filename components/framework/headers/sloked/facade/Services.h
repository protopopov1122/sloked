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

#ifndef SLOKED_FACADE_SERVICES_H_
#define SLOKED_FACADE_SERVICES_H_

#include "sloked/kgr/local/Context.h"
#include "sloked/kgr/ctx-manager/RunnableContextManager.h"
#include "sloked/core/Logger.h"
#include "sloked/kgr/NamedServer.h"
#include "sloked/namespace/Mount.h"
#include "sloked/core/CharWidth.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"
#include "sloked/namespace/Root.h"

namespace sloked {

    class SlokedServiceDependencyProvider : public SlokedCloseable {
     public:
        virtual ~SlokedServiceDependencyProvider() = default;
        virtual KgrContextManager<KgrLocalContext> &GetContextManager() = 0;
        virtual SlokedTextTaggerRegistry<int> &GetTaggers() = 0;
        virtual SlokedLogger &GetLogger() = 0;
        virtual SlokedRootNamespace &GetNamespace() = 0;
        virtual const SlokedCharWidth &GetCharWidth() = 0;
        virtual KgrNamedServer &GetServer() = 0;
        virtual SlokedEditorDocumentSet &GetDocuments() = 0;
        virtual void Start() = 0;
    };

    class SlokedServiceDependencyDefaultProvider : public SlokedServiceDependencyProvider {
     public:
        SlokedServiceDependencyDefaultProvider(SlokedLogger &, std::unique_ptr<SlokedRootNamespace>, const SlokedCharWidth &, KgrNamedServer &);
        KgrContextManager<KgrLocalContext> &GetContextManager() override;
        SlokedTextTaggerRegistry<int> &GetTaggers() override;
        SlokedLogger &GetLogger() override;
        SlokedRootNamespace &GetNamespace() override;
        const SlokedCharWidth &GetCharWidth() override;
        KgrNamedServer &GetServer() override;
        SlokedEditorDocumentSet &GetDocuments() override;
        void Close() override;
        void Start() override;

     protected:
        SlokedLogger &logger;
        std::unique_ptr<SlokedRootNamespace> rootNamespace;
        const SlokedCharWidth &charWidth;
        KgrNamedServer &server;
        SlokedEditorDocumentSet documents;
        KgrRunnableContextManagerHandle<KgrLocalContext> contextManager;
        SlokedTextTaggerRegistry<int> taggers;
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
        std::map<std::string, std::function<std::unique_ptr<KgrService>(SlokedServiceDependencyProvider &)>> builders;
    };
}

#endif