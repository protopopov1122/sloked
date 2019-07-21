#include "sloked/text/TextRegion.h"
#include "sloked/core/Error.h"
#include <iostream>

namespace sloked {


    TextRegion::TextRegion(std::unique_ptr<TextBlock> content)
        : AVLNode(nullptr, nullptr),
          content(std::move(content)),
          height(0), last_line(0) {
        this->UpdateStats();
    }

    std::size_t TextRegion::GetLastLine() const {
        return this->last_line;
    }

    std::size_t TextRegion::GetTotalLength() const {
        std::size_t length = 0;
        if (this->begin) {
            length = this->begin->GetTotalLength();
        }
        if (this->content) {
            if (this->begin) {
                length++;
            }
            length += this->content->GetTotalLength();
        }
        if (this->end) {
            if (this->begin || this->content) {
                length++;
            }
            length += this->end->GetTotalLength();
        }
        return length;
    }

    const std::string_view TextRegion::GetLine(std::size_t line) const {
        const std::size_t begin_length = this->begin ? this->begin->GetLastLine() + 1 : 0;
        const std::size_t self_length = this->content ? this->content->GetLastLine() + 1 : 0;
        if (this->begin && line <= this->begin->GetLastLine()) {
            return this->begin->GetLine(line);
        } else if (this->content && line <= begin_length + this->content->GetLastLine()) {
            return this->content->GetLine(line - begin_length);
        } else if (this->end && line <= begin_length + self_length + this->end->GetLastLine()) {
            return this->end->GetLine(line - begin_length - self_length);
        } else {
            throw SlokedError("Line " + std::to_string(line) + " exceeds total length of block");
        }
    }

    bool TextRegion::Empty() const {
        return (!this->content || this->content->Empty()) &&
            (!this->begin || this->begin->Empty()) &&
            (!this->end || this->end->Empty());
    }

    void TextRegion::SetLine(std::size_t line, const std::string &content) {
        const std::size_t begin_length = this->begin ? this->begin->GetLastLine() + 1 : 0;
        const std::size_t self_length = this->content ? this->content->GetLastLine() + 1 : 0;
        if (this->begin && line <= this->begin->GetLastLine()) {
            this->begin->SetLine(line, content);
        } else if (this->content && line <= begin_length + this->content->GetLastLine()) {
            this->content->SetLine(line - begin_length, content);
        } else if (this->end && line <= begin_length + self_length + this->end->GetLastLine()) {
            this->end->SetLine(line - begin_length - self_length, content);
        } else {
            throw SlokedError("Line " + std::to_string(line) + " exceeds total length of block");
        }
    }

    void TextRegion::EraseLine(std::size_t line) {
        const std::size_t begin_length = this->begin ? this->begin->GetLastLine() + 1 : 0;
        const std::size_t self_length = this->content ? this->content->GetLastLine() + 1 : 0;
        if (this->begin && line <= this->begin->GetLastLine()) {
            this->begin->EraseLine(line);
            if (this->begin->Empty()) {
                this->begin.reset();
            }
        } else if (this->content && line <= begin_length + this->content->GetLastLine()) {
            this->content->EraseLine(line - begin_length);
            if (this->content->Empty()) {
                this->content.reset();
            }
        } else if (this->end && line <= begin_length + self_length + this->end->GetLastLine()) {
            this->end->EraseLine(line - begin_length - self_length);
            if (this->end->Empty()) {
                this->end.reset();
            }
        } else {
            throw SlokedError("Line " + std::to_string(line) + " exceeds total length of block");
        }
        this->AvlBalance();
        this->UpdateStats();
    }

    void TextRegion::InsertLine(std::size_t line, const std::string &content) {
        const std::size_t begin_length = this->begin ? this->begin->GetLastLine() + 1 : 0;
        const std::size_t self_length = this->content ? this->content->GetLastLine() + 1 : 0;
        if (this->begin && line <= this->begin->GetLastLine()) {
            this->begin->InsertLine(line, content);
        } else if (this->content && line <= begin_length + this->content->GetLastLine()) {
            this->content->InsertLine(line - begin_length, content);
        } else if (this->end && line <= begin_length + self_length + this->end->GetLastLine()) {
            this->end->InsertLine(line - begin_length - self_length, content);
        } else {
            throw SlokedError("Line " + std::to_string(line) + " exceeds total length of block");
        }
        this->UpdateStats();
    }

    void TextRegion::Optimize() {
        if (this->begin) {
            this->begin->Optimize();
        }
        if (this->content) {
            this->content->Optimize();
        }
        if (this->end) {
            this->end->Optimize();
        }
        this->AvlBalance();
    }

    std::ostream &operator<<(std::ostream &os, const TextRegion &region) {
        if (region.begin) {
            os << *region.begin;
        }
        if (region.content) {
            if (region.begin) {
                os << std::endl;
            }
            os << *region.content;
        }
        if (region.end) {
            if (region.begin || region.content) {
                os << std::endl;
            }
            os << *region.end;
        }
        return os;
    }

    void TextRegion::AppendRegion(std::unique_ptr<TextRegion> region) {
        if (this->end) {
            this->end->AppendRegion(std::move(region));
            this->AvlBalance();
        } else {
            this->end = std::move(region);
        }
        this->UpdateStats();
    }

    std::size_t TextRegion::GetHeight() const {
        return this->height;
    }

    void TextRegion::AvlSwapContent(TextRegion &region) {
        std::swap(this->content, region.content);
    }

    void TextRegion::AvlUpdate() {
        if (this->begin && this->begin->Empty()) {
            this->begin.reset();
        }
        if (this->content && this->content->Empty()) {
            this->content.reset();
        }
        if (this->end && this->end->Empty()) {
            this->end.reset();
        }
        this->UpdateStats();
    }

#define HEIGHT(p) (p != nullptr ? p->height + 1 : 0)
    void TextRegion::UpdateStats() const {
        this->height = std::max(HEIGHT(this->begin), HEIGHT(this->end));

        this->last_line = 0;
        if (this->begin) {
            this->last_line = this->begin->GetLastLine();
        }
        if (this->content) {
            if (this->begin) {
                this->last_line++;
            }
            this->last_line += this->content->GetLastLine();
        }
        if (this->end) {
            if (this->begin || this->content) {
                this->last_line++;
            }
            this->last_line += this->end->GetLastLine();
        }
    }
#undef HEIGHT
}