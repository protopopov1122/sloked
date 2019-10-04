#include "sloked/editor/Document.h"
#include "sloked/text/TextView.h"

namespace sloked {

    SlokedEditorDocument::SlokedEditorDocument(SlokedNamespace &ns, const SlokedPath &path, const Encoding &encoding, std::unique_ptr<NewLine> newline)
        : fileView(ns.GetObject(path)->AsFile()->View()), blockFactory(*newline), text(*newline, TextView::Open(*fileView, *newline, blockFactory)),
          encoding(encoding), newline(std::move(newline)), multiplexer(text, encoding) {}

    TextBlock &SlokedEditorDocument::GetText() {
        return this->text;
    }

    const Encoding &SlokedEditorDocument::GetEncoding() {
        return this->encoding;
    }

    std::unique_ptr<SlokedTransactionStream> SlokedEditorDocument::NewStream() {
        return this->multiplexer.NewStream();
    }

    SlokedTransactionListenerManager &SlokedEditorDocument::GetTransactionListeners() {
        return this->multiplexer;
    }
}