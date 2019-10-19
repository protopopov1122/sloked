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

#ifndef SLOKED_EDITOR_DOCMGR_DOCUMENT_H_
#define SLOKED_EDITOR_DOCMGR_DOCUMENT_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/NewLine.h"
#include "sloked/text/TextDocument.h"
#include "sloked/text/TextChunk.h"
#include "sloked/text/cursor/TransactionStreamMultiplexer.h"
#include "sloked/namespace/Object.h"
#include "sloked/editor/doc-mgr/DocumentUpstream.h"

namespace sloked {

    class SlokedEditorDocument {
     public:
        SlokedEditorDocument(const Encoding &, std::unique_ptr<NewLine>);
        SlokedEditorDocument(SlokedNamespace &, const SlokedPath &, const Encoding &, std::unique_ptr<NewLine>);
        std::optional<std::reference_wrapper<const SlokedPath>> GetUpstream() const;
        bool HasUpstream() const;
        TextBlock &GetText();
        const Encoding &GetEncoding();
        std::unique_ptr<SlokedTransactionStream> NewStream();
        SlokedTransactionListenerManager &GetTransactionListeners();
        void Save();
        void Save(SlokedNamespace &, const SlokedPath &);

     private:
        TextChunkFactory blockFactory;
        SlokedDocumentUpstream upstream;
        std::reference_wrapper<const Encoding> encoding;
        std::unique_ptr<NewLine> newline;
        TransactionStreamMultiplexer multiplexer;
    };
}

#endif