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

#include "sloked/text/TextRegion.h"
#include "sloked/core/Error.h"
#include "sloked/core/NewLine.h"
#include <iostream>
#include <algorithm>

namespace sloked {


    TextRegion::TextRegion(const NewLine &newline, std::unique_ptr<TextBlock> content)
        : AVLNode(nullptr, nullptr),
          newline(newline),
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

    std::string_view TextRegion::GetLine(std::size_t line) const {
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

    void TextRegion::Visit(std::size_t start, std::size_t count, Visitor visitor) const {
        const std::size_t begin_length = this->begin ? this->begin->GetLastLine() + 1 : 0;
        const std::size_t self_length = this->content ? this->content->GetLastLine() + 1 : 0;
        if (count && this->begin && start <= this->begin->GetLastLine()) {
            const std::size_t read = std::min(count, this->begin->GetLastLine() - start + 1);
            this->begin->Visit(start, read, visitor);
            count -= read;
            start += read;
        }
        if (count && this->content && start <= begin_length + this->content->GetLastLine()) {
            const std::size_t node_start = start - begin_length;
            const std::size_t read = std::min(count, this->content->GetLastLine() - node_start + 1);
            this->content->Visit(node_start, read, visitor);
            count -= read;
            start += read;
        }
        if (count && this->end && start <= begin_length + self_length + this->end->GetLastLine()) {
            const std::size_t node_start = start - begin_length - self_length;
            const std::size_t read = std::min(count, this->end->GetLastLine() - node_start + 1);
            this->end->Visit(node_start, read, visitor);
            count -= read;
        }
        if (count) {
            throw SlokedError("Range exceeds total length of block");
        }
    }

    void TextRegion::SetLine(std::size_t line, std::string_view content) {
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

    void TextRegion::InsertLine(std::size_t line, std::string_view content) {
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
        if (region.begin && !region.begin->Empty()) {
            os << *region.begin;
        }
        if (region.content) {
            if (region.begin && !region.begin->Empty()) {
                os << region.newline;
            }
            os << *region.content;
        }
        if (region.end && !region.end->Empty()) {
            if ((region.begin && !region.begin->Empty()) ||
                (region.content && !region.content->Empty())) {
                os << region.newline;
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