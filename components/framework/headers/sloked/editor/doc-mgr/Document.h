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

#ifndef SLOKED_EDITOR_DOCMGR_DOCUMENT_H_
#define SLOKED_EDITOR_DOCMGR_DOCUMENT_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/NewLine.h"
#include "sloked/text/TextDocument.h"
#include "sloked/text/TextChunk.h"
#include "sloked/text/cursor/TransactionStreamMultiplexer.h"
#include "sloked/namespace/Object.h"
#include "sloked/editor/doc-mgr/DocumentUpstream.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/core/Event.h"

namespace sloked {

    class SlokedEditorDocument : public SlokedTaggableDocument {
     public:
        using TagType = int;
        SlokedEditorDocument(const Encoding &, std::unique_ptr<NewLine>);
        SlokedEditorDocument(SlokedNamespace &, const SlokedPath &, const Encoding &, std::unique_ptr<NewLine>);
        std::optional<SlokedPath> GetUpstream() const final;
        std::optional<std::string> GetUpstreamURI() const final;
        bool HasUpstream() const final;
        TextBlock &GetText();
        const TextBlockView &GetText() const final;
        const Encoding &GetEncoding() const final;
        std::unique_ptr<SlokedTransactionStream> NewStream();
        SlokedTransactionListenerManager &GetTransactionListeners() final;
        void AttachTagger(std::unique_ptr<SlokedTextTagger<TagType>>);
        SlokedTextTagger<TagType> &GetTagger();
        void Save();
        void Save(SlokedNamespace &, const SlokedPath &);

     private:
        TextChunkFactory blockFactory;
        SlokedDocumentUpstream upstream;
        std::reference_wrapper<const Encoding> encoding;
        std::unique_ptr<NewLine> newline;
        TransactionStreamMultiplexer multiplexer;
        SlokedTextProxyTagger<TagType> tagger;
    };
}

#endif