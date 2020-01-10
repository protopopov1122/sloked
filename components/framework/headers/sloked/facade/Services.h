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

namespace sloked {

    class SlokedAbstractServicesFacade : public SlokedCloseable {
     public:
        SlokedAbstractServicesFacade(SlokedLogger &, SlokedMountableNamespace &, SlokedNamespaceMounter &, const SlokedCharWidth &);
        virtual ~SlokedAbstractServicesFacade() = default;
        KgrContextManager<KgrLocalContext> &GetContextManager();
        SlokedTextTaggerRegistry<int> &GetTaggers();
        void Start();
        virtual void Close() override;
        virtual void Apply(KgrNamedServer &) = 0;

     protected:
        SlokedLogger &logger;
        SlokedMountableNamespace &root;
        SlokedNamespaceMounter &mounter;
        const SlokedCharWidth &charWidth;
        SlokedEditorDocumentSet documents;
        KgrRunnableContextManagerHandle<KgrLocalContext> contextManager;
        SlokedTextTaggerRegistry<int> taggers;
    };

    class SlokedDefaultServicesFacade : public SlokedAbstractServicesFacade {
     public:
        using SlokedAbstractServicesFacade::SlokedAbstractServicesFacade;
        void Apply(KgrNamedServer &) final;
    };
}

#endif