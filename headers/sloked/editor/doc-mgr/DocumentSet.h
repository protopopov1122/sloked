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

#ifndef SLOKED_EDITOR_DOCMGR_DOCUMENTSET_H_
#define SLOKED_EDITOR_DOCMGR_DOCUMENTSET_H_

#include "sloked/editor/doc-mgr/Document.h"
#include "sloked/core/Resource.h"

namespace sloked {

	class SlokedEditorDocumentSet {
	 public:
		using Document = SlokedRegistry<SlokedEditorDocument>::Resource;
		using DocumentId = SlokedRegistry<SlokedEditorDocument>::Key;

		SlokedEditorDocumentSet(SlokedNamespace &);
		Document NewDocument(const Encoding &, std::unique_ptr<NewLine>);
		Document OpenDocument(const SlokedPath &, const Encoding &, std::unique_ptr<NewLine>);
		std::optional<Document> OpenDocument(DocumentId);
		void SaveAs(SlokedEditorDocument &, const SlokedPath &);
		bool HasDocument(DocumentId);
		Document Empty();

	 private:
		SlokedNamespace &ns;
		SlokedRegistry<SlokedEditorDocument> documents;
	};
}

#endif
