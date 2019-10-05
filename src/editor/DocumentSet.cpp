#include "sloked/editor/DocumentSet.h"

namespace sloked {

    SlokedEditorDocumentSet::SlokedEditorDocumentSet(SlokedNamespace &ns)
        : ns(ns) {}

    SlokedEditorDocumentSet::Document SlokedEditorDocumentSet::OpenDocument(const SlokedPath &path, const Encoding &encoding, std::unique_ptr<NewLine> newline) {
        auto document = std::make_unique<SlokedEditorDocument>(this->ns, path, encoding, std::move(newline));
        auto handle = this->documents.Add(std::move(document));
        return handle;
    }

    std::optional<SlokedEditorDocumentSet::Document> SlokedEditorDocumentSet::OpenDocument(DocumentId id) {
        return this->documents.Get(id);
    }

    bool SlokedEditorDocumentSet::HasDocument(DocumentId id) {
        return this->documents.Has(id);
    }
}