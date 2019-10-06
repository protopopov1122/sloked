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

#ifndef SLOKED_EDITOR_DOCUMENT_H_
#define SLOKED_EDITOR_DOCUMENT_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/NewLine.h"
#include "sloked/text/TextDocument.h"
#include "sloked/text/TextChunk.h"
#include "sloked/text/cursor/TransactionStreamMultiplexer.h"
#include "sloked/namespace/Object.h"

namespace sloked {

    class SlokedEditorDocument {
     public:
        SlokedEditorDocument(SlokedNamespace &, const SlokedPath &, const Encoding &, std::unique_ptr<NewLine>);
        TextBlock &GetText();
        const Encoding &GetEncoding();
        std::unique_ptr<SlokedTransactionStream> NewStream();
        SlokedTransactionListenerManager &GetTransactionListeners();

     private:
        std::unique_ptr<SlokedIOView> fileView;
        TextChunkFactory blockFactory;
        TextDocument text;
        const Encoding &encoding;
        std::unique_ptr<NewLine> newline;
        TransactionStreamMultiplexer multiplexer;
    };
}

#endif