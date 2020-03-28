#include "sloked/editor/doc-mgr/DocumentUpstream.h"

#include <sstream>

#include "sloked/core/Error.h"
#include "sloked/core/Position.h"
#include "sloked/text/TextChunk.h"
#include "sloked/text/TextDocument.h"
#include "sloked/text/TextView.h"

namespace sloked {

    SlokedDocumentUpstream::SlokedDocumentUpstream(
        const TextBlockFactory &blockFactory, const NewLine &newline)
        : blockFactory(std::cref(blockFactory)), newline(std::cref(newline)) {
        this->content = std::make_unique<TextDocument>(
            this->newline, std::make_unique<TextChunk>(newline, ""));
    }

    SlokedDocumentUpstream::SlokedDocumentUpstream(
        SlokedNamespace &ns, const SlokedPath &path,
        const TextBlockFactory &blockFactory, const NewLine &newline)
        : blockFactory(std::cref(blockFactory)), newline(std::cref(newline)) {
        auto file = ns.GetObject(path);
        if (file->AsFile() == nullptr) {
            throw SlokedError("Not a file: " + path.ToString());
        }
        auto view = file->AsFile()->View();
        this->upstream = Upstream{std::move(file), std::move(view),
                                  ns.GetHandle(path)->ToURI()};
        this->content = std::make_unique<TextDocument>(
            this->newline, TextView::Open(*this->upstream.value().fileView,
                                          this->newline, this->blockFactory));
    }

    TextBlock &SlokedDocumentUpstream::GetText() {
        return *this->content;
    }

    const TextBlockView &SlokedDocumentUpstream::GetText() const {
        return *this->content;
    }

    std::optional<std::reference_wrapper<const SlokedPath>>
        SlokedDocumentUpstream::GetUpstream() const {
        if (this->upstream.has_value()) {
            return std::cref(this->upstream.value().file->GetPath());
        } else {
            return {};
        }
    }

    std::optional<std::string> SlokedDocumentUpstream::GetUpstreamURI() const {
        if (this->upstream.has_value()) {
            return this->upstream.value().uri;
        } else {
            return {};
        }
    }

    bool SlokedDocumentUpstream::HasUpstream() const {
        return this->upstream.has_value();
    }

    void SlokedDocumentUpstream::Save() {
        if (!this->HasUpstream()) {
            throw SlokedError("Can't save document without upstream");
        }
        std::stringstream content;
        content << *this->content;
        auto writer = this->upstream.value().file->AsFile()->Writer();
        writer->Write(content.str());
        writer->Flush();

        this->upstream.value().fileView =
            this->upstream.value().file->AsFile()->View();
        this->content->Rebuild(
            this->newline, TextView::Open(*this->upstream.value().fileView,
                                          this->newline, this->blockFactory));
    }

    void SlokedDocumentUpstream::Save(SlokedNamespace &ns,
                                      const SlokedPath &path,
                                      const TextBlockFactory &blockFactory,
                                      const NewLine &newline) {
        auto fileHandle = ns.GetHandle(path);
        if (!fileHandle->Exists()) {
            fileHandle->MakeFile();
        }
        auto file = ns.GetObject(path);
        if (file == nullptr || file->AsFile() == nullptr) {
            throw SlokedError("Not a file: " + path.ToString());
        }

        std::stringstream content;
        content << *this->content;
        auto writer = file->AsFile()->Writer();
        writer->Write(content.str());
        writer->Flush();

        this->blockFactory = std::cref(blockFactory);
        this->newline = std::cref(this->newline);
        auto view = file->AsFile()->View();
        this->upstream =
            Upstream{std::move(file), std::move(view), fileHandle->ToURI()};
        this->content->Rebuild(
            this->newline, TextView::Open(*this->upstream.value().fileView,
                                          this->newline, this->blockFactory));
    }
}  // namespace sloked