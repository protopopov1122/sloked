#ifndef SLOKED_EDITOR_DOCUMENTSET_H_
#define SLOKED_EDITOR_DOCUMENTSET_H_

#include "sloked/editor/Document.h"
#include "sloked/core/Resource.h"

namespace sloked {

	class SlokedEditorDocumentSet {
	 public:
		using Document = SlokedRegistry<SlokedEditorDocument>::Resource;
		using DocumentId = SlokedRegistry<SlokedEditorDocument>::Key;

		SlokedEditorDocumentSet(SlokedNamespace &);
		Document OpenDocument(const SlokedPath &, const Encoding &, std::unique_ptr<NewLine>);
		std::optional<Document> OpenDocument(DocumentId);
		bool HasDocument(DocumentId);

	 private:
		SlokedNamespace &ns;
		SlokedRegistry<SlokedEditorDocument> documents;
	};
}

#endif
