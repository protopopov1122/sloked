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

#include "sloked/editor/doc-mgr/Document.h"
#include "sloked/text/TextView.h"

namespace sloked {

    SlokedEditorDocument::SlokedEditorDocument(const Encoding &encoding, std::unique_ptr<NewLine> newline)
        : blockFactory(*newline), upstream(this->blockFactory, *newline),
          encoding(std::cref(encoding)), newline(std::move(newline)), multiplexer(this->upstream.GetText(), encoding) {}

    SlokedEditorDocument::SlokedEditorDocument(SlokedNamespace &ns, const SlokedPath &path, const Encoding &encoding, std::unique_ptr<NewLine> newline)
        : blockFactory(*newline), upstream(ns, path, this->blockFactory, *newline),
          encoding(encoding), newline(std::move(newline)), multiplexer(this->upstream.GetText(), encoding) {}

    std::optional<SlokedPath> SlokedEditorDocument::GetUpstream() const {
        return this->upstream.GetUpstream();
    }
    
    std::optional<std::string> SlokedEditorDocument::GetUpstreamURI() const {
        return this->upstream.GetUpstreamURI();
    }

    bool SlokedEditorDocument::HasUpstream() const {
        return this->upstream.HasUpstream();
    }

    TextBlock &SlokedEditorDocument::GetText() {
        return this->upstream.GetText();
    }

    const TextBlockView &SlokedEditorDocument::GetText() const {
        return this->upstream.GetText();
    }

    const Encoding &SlokedEditorDocument::GetEncoding() const {
        return this->encoding;
    }

    std::unique_ptr<SlokedTransactionStream> SlokedEditorDocument::NewStream() {
        return this->multiplexer.NewStream();
    }

    SlokedTransactionListenerManager &SlokedEditorDocument::GetTransactionListeners() {
        return this->multiplexer;
    }

    void SlokedEditorDocument::Save() {
        this->upstream.Save();
    }

    void SlokedEditorDocument::Save(SlokedNamespace &ns, const SlokedPath &path) {
        this->upstream.Save(ns, path, this->blockFactory, *this->newline);
    }
}