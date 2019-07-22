#include "sloked/text/TextDocument.h"
#include "sloked/text/TextChunk.h"
#include "sloked/core/Error.h"

namespace sloked {

    TextDocument::TextDocument(const NewLine &newline, std::unique_ptr<TextBlock> content)
        : newline(newline), content(std::move(content)) {}

    std::size_t TextDocument::GetLastLine() const {
        if (this->content) {
            return this->content->GetLastLine();
        } else {
            return 0;
        }
    }
    
    std::size_t TextDocument::GetTotalLength() const {
        if (this->content) {
            return this->content->GetTotalLength();
        } else {
            return 0;
        }
    }

    const std::string_view TextDocument::GetLine(std::size_t line) const {
        if (this->content) {
            return this->content->GetLine(line);
        } else if (line == 0) {
            return "";
        } else {
            throw SlokedError("Line " + std::to_string(line) + " exceeds total length of block");
        }
    }

    bool TextDocument::Empty() const {
        if (this->content) {
            return this->content->Empty();
        } else {
            return true;
        }
    }

    void TextDocument::Visit(std::size_t start, std::size_t count, Visitor visitor) const {
        if (this->content) {
            this->content->Visit(start, count, visitor);
        } else if (start == 0 && count == 1) {
            visitor("");
        } else {
            throw SlokedError("Range exceeds total length of block");
        }
    }
    
    void TextDocument::SetLine(std::size_t line, const std::string &content) {
        if (this->content) {
            this->content->SetLine(line, content);
        } else if (line == 0) {
            this->content = std::make_unique<TextChunk>(this->newline, content);
        } else {
            throw SlokedError("Line " + std::to_string(line) + " exceeds total length of block");
        }
    }

    void TextDocument::EraseLine(std::size_t line) {
        if (this->content) {
            this->content->EraseLine(line);
            if (this->content->Empty()) {
                this->content.reset();
            }
        } else if (line > 0) {
            throw SlokedError("Line " + std::to_string(line) + " exceeds total length of block");
        }
    }

    void TextDocument::InsertLine(std::size_t line, const std::string &content) {
        if (this->content) {
            this->content->InsertLine(line, content);
        } else if (line == 0) {
            this->content = std::make_unique<TextChunk>(this->newline, "\n" + content);
        } else {
            throw SlokedError("Line " + std::to_string(line) + " exceeds total length of block");
        }
    }
    
    void TextDocument::Optimize() {
        if (this->content) {
            this->content->Optimize();
        }
    }

    std::ostream &operator<<(std::ostream &os, const TextDocument &doc) {
        if (doc.content) {
            return os << *doc.content;
        } else {
            return os;
        }
    }
}