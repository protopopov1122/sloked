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

#ifndef SLOKED_SERVICES_CURSOR_H_
#define SLOKED_SERVICES_CURSOR_H_

#include "sloked/core/CharPreset.h"
#include "sloked/core/Encoding.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"
#include "sloked/kgr/ContextManager.h"
#include "sloked/kgr/NamedServer.h"
#include "sloked/kgr/Service.h"
#include "sloked/services/Service.h"
#include "sloked/text/TextBlock.h"
#include "sloked/text/cursor/TransactionStreamMultiplexer.h"
#include "sloked/text/fragment/TaggedText.h"

namespace sloked {

    class SlokedCursorService : public KgrService {
     public:
        SlokedCursorService(SlokedEditorDocumentSet &, KgrServer::Connector,
                            KgrContextManager<KgrLocalContext> &);
        void Attach(std::unique_ptr<KgrPipe>) override;

     private:
        SlokedEditorDocumentSet &documents;
        KgrServer::Connector renderConnector;
        KgrContextManager<KgrLocalContext> &contextManager;
    };

    class SlokedCursorClient {
     public:
        SlokedCursorClient(std::unique_ptr<KgrPipe>);
        std::unique_ptr<SlokedDeferredTask> Connect(
            SlokedEditorDocumentSet::DocumentId, std::function<void(bool)>);
        void Insert(const std::string &);
        void MoveUp();
        void MoveDown();
        void MoveBackward();
        void MoveForward();
        void NewLine();
        void DeleteBackward();
        void DeleteForward();
        void Undo();
        void Redo();
        std::optional<TextPosition> GetPosition();

     private:
        SlokedServiceClient client;
    };
}  // namespace sloked

#endif