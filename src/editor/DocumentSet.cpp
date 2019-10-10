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

#include "sloked/editor/DocumentSet.h"

namespace sloked {

    SlokedEditorDocumentSet::SlokedEditorDocumentSet(SlokedNamespace &ns)
        : ns(ns) {}

    SlokedEditorDocumentSet::Document SlokedEditorDocumentSet::NewDocument(const Encoding &encoding, std::unique_ptr<NewLine> newline) {
        auto document = std::make_unique<SlokedEditorDocument>(encoding, std::move(newline));
        auto handle = this->documents.Add(std::move(document));
        return handle;
    }

    SlokedEditorDocumentSet::Document SlokedEditorDocumentSet::OpenDocument(const SlokedPath &path, const Encoding &encoding, std::unique_ptr<NewLine> newline) {
        auto document = std::make_unique<SlokedEditorDocument>(this->ns, path, encoding, std::move(newline));
        auto handle = this->documents.Add(std::move(document));
        return handle;
    }

    std::optional<SlokedEditorDocumentSet::Document> SlokedEditorDocumentSet::OpenDocument(DocumentId id) {
        return this->documents.Get(id);
    }

    void SlokedEditorDocumentSet::SaveAs(SlokedEditorDocument &doc, const SlokedPath &path) {
        doc.Save(this->ns, path);
    }

    bool SlokedEditorDocumentSet::HasDocument(DocumentId id) {
        return this->documents.Has(id);
    }

    SlokedEditorDocumentSet::Document SlokedEditorDocumentSet::Empty() {
        return Document(this->documents);
    }
}