/*
  SPDX-License-Identifier: LGPL-3.0-or-later

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/text/TextChunk.h"
#include "sloked/core/Error.h"
#include "sloked/core/NewLine.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace sloked {

    TextChunkFactory::TextChunkFactory(const NewLine &newline)
        : newline(newline) {}

    std::unique_ptr<TextBlock> TextChunkFactory::make(std::string_view content) const {
        return std::make_unique<TextChunk>(this->newline, content);
    }

    TextChunk::TextChunk(const NewLine &newline, std::string_view content, std::size_t first_line)
        : AVLNode(nullptr, nullptr),
          newline(newline), has_content(true), height(0), first_line(first_line), content(content) {
        this->Remap();
    }

    TextChunk::TextChunk(const NewLine &newline, std::unique_ptr<TextChunk> begin, std::optional<std::string> content, std::unique_ptr<TextChunk> end, std::size_t first_line)
        : AVLNode(std::move(begin), std::move(end)),
          newline(newline), has_content(false), height(0), first_line(first_line), content(std::move(content)) {
        this->Remap();
        this->UpdateStats();
    }

    std::size_t TextChunk::GetLastLine() const {
        return this->chunk_map->GetLastLine();
    }

    std::size_t TextChunk::GetTotalLength() const {
        return this->chunk_map->GetTotalLength();
    }

    std::string_view TextChunk::GetLine(std::size_t line) const {
        line -= this->first_line;
        std::size_t offset = this->chunk_map->GetOffset(line);
        switch (offset) {
            case DefaultMap::NotFound:
                throw SlokedError("Line " + std::to_string(line) + " exceeds total length of block");
            case DefaultMap::AtBegin:
                return this->begin->GetLine(line);
            case DefaultMap::AtEnd:
                return this->end->GetLine(line);
            default:
                return std::string_view{this->content.value()}.substr(offset, this->chunk_map->GetLength(line));
        }
    }

    bool TextChunk::Empty() const {
        return !this->has_content;
    }

    void TextChunk::Visit(std::size_t start, std::size_t count, Visitor visitor) const {
        start -= this->first_line;
        std::size_t begin_count = 0, end_count = 0;
        for (; begin_count < count && this->chunk_map->GetOffset(start + begin_count) == DefaultMap::AtBegin; begin_count++) {}
        for (; end_count < count && this->chunk_map->GetOffset(start + count - end_count - 1) == DefaultMap::AtEnd; end_count++) {}
        if (begin_count) {
            this->begin->Visit(start, begin_count, visitor);
        }
        if (count - (begin_count + end_count)) {
            std::string_view view(this->content.value());
            for (std::size_t i = start + begin_count; i < start + begin_count + count - (begin_count + end_count); i++) {
                std::size_t offset = this->chunk_map->GetOffset(i);
                if (offset == DefaultMap::NotFound) {
                    throw SlokedError("Line " + std::to_string(i) + " exceeds total length of block");
                }
                std::size_t length = this->chunk_map->GetLength(i);
                visitor(view.substr(offset, length));
            }
        }
        if (end_count) {
            this->end->Visit(start + count - end_count, end_count, visitor);
        }
    }

    void TextChunk::SetLine(std::size_t line, std::string_view content) {
        line -= this->first_line;
        std::size_t offset = this->chunk_map->GetOffset(line);
        switch (offset) {
            case DefaultMap::AtBegin:
                this->begin->SetLine(line, content);
                break;

            case DefaultMap::AtEnd:
                this->end->SetLine(line, content);
                break;

            case DefaultMap::NotFound:
                throw SlokedError("Line " + std::to_string(line) + " exceeds total length of block");

            default: {
                std::size_t length = this->chunk_map->GetLength(line);
                if (offset == 0 && length == this->content.value().size()) {
                    this->content = content;
                } else {
                    std::string_view view {this->content.value()};
                    if (this->end == nullptr) {
                        this->end = line < this->GetLastLine()
                            ? std::make_unique<TextChunk>(this->newline, view.substr(offset + length + this->newline.Width), line + 1)
                            : nullptr;
                        this->content.value().erase(offset + length);
                        view = this->content.value();
                    }
                    if (this->begin == nullptr) {
                        this->begin = line > 0
                            ? std::make_unique<TextChunk>(this->newline, view.substr(0, offset - this->newline.Width))
                            : nullptr;
                        this->content.value().erase(0, offset);
                        view = this->content.value();
                        offset = 0;
                    }
                    this->content.value().replace(offset, length, content);
                    this->UpdateStats();
                }
            } break;
        }
        this->Normalize();
        this->Remap();
    }

    void TextChunk::EraseLine(std::size_t line) {
        line -= this->first_line;
        std::size_t offset = this->chunk_map->GetOffset(line);
        switch (offset) {
            case DefaultMap::AtBegin:
                this->begin->EraseLine(line);
                if (this->end) {
                    this->end->first_line--;
                    this->end->Remap();
                }
                break;

            case DefaultMap::AtEnd:
                this->end->EraseLine(line);
                break;

            case DefaultMap::NotFound:
                throw SlokedError("Line " + std::to_string(line) + " exceeds total length of block");

            default: {
                std::size_t length = this->chunk_map->GetLength(line);
                if (offset == 0 && length == this->content.value().size()) {
                    this->content.reset();
                    if (this->end) {
                        this->end->first_line--;
                        this->end->Remap();
                    }
                } else {
                    std::string_view view {this->content.value()};
                    if (this->end == nullptr) {
                        this->end = line < this->GetLastLine()
                            ? std::make_unique<TextChunk>(this->newline, view.substr(offset + length + this->newline.Width), line + 1)
                            : nullptr;
                        this->content.value().erase(offset + length);
                        view = this->content.value();
                    }
                    if (this->begin == nullptr) {
                        this->begin = line > 0
                            ? std::make_unique<TextChunk>(this->newline, view.substr(0, offset - this->newline.Width))
                            : nullptr;
                        this->content.value().erase(0, offset);
                        offset = 0;
                    }
                    this->content.value().erase(offset, length + this->newline.Width);
                    if (this->content.value().empty()) {
                        this->content.reset();
                        if (this->end) {
                            this->end->first_line--;
                            this->end->Remap();
                        }
                    }
                }
            } break;
        }
        this->Remap();
        this->Normalize();
        this->UpdateStats();
    }

    void TextChunk::InsertLine(std::size_t line, std::string_view content) {
        line -= this->first_line;
        std::size_t offset = this->chunk_map->GetOffset(line);
        switch (offset) {
            case DefaultMap::AtBegin:
                this->begin->InsertLine(line, content);
                if (this->end) {
                    this->end->first_line++;
                    this->end->Remap();
                }
                break;

            case DefaultMap::AtEnd:
                this->end->InsertLine(line, content);
                break;

            case DefaultMap::NotFound:
                throw SlokedError("Line " + std::to_string(line) + " exceeds total length of block");
                
            default:
                std::size_t length = this->chunk_map->GetLength(line);
                std::unique_ptr<TextChunk> tail = nullptr;
                if (offset + length < this->content.value().size()) {
                    tail = std::make_unique<TextChunk>(this->newline, nullptr, this->content.value().substr(offset + length + this->newline.Width), std::move(this->end));
                    this->content.value().erase(offset + length);
                } else {
                    tail = std::move(this->end);
                }
                if (tail) {
                    tail->Normalize();
                }
                this->end = std::make_unique<TextChunk>(this->newline, nullptr, std::string {content}, std::move(tail), line + 1);
                this->end->Normalize();
                break;
        }
        this->Remap();
        this->Normalize();
        this->UpdateStats();
    }

    void TextChunk::Optimize() {
        this->Compact();
        this->Balance();
    }

    void TextChunk::Squash() {
        std::stringstream ss;
        ss << *this;
        this->content = ss.str();
        this->begin.reset();
        this->end.reset();
        this->Remap();
        this->height = 0;
    }

    void TextChunk::Compact() {
        if (this->begin) {
            if (!this->begin->Empty()) {
                this->begin->Compact();
            } else {
                this->begin.reset();
            }
        }
        if (this->end) {
            if (!this->end->Empty()) {
                this->end->Compact();
            } else {
                this->end.reset();
            }
        }
        this->UpdateStats();
    }

    void TextChunk::Balance() {
        this->AvlBalance();
    }

    std::size_t TextChunk::GetHeight() const {
        return this->height;
    }

    void TextChunk::AvlSwapContent(TextChunk &chunk) {
        std::swap(this->content, chunk.content);
    }

    void TextChunk::AvlUpdate() {
        this->Normalize();
        this->Remap();
        this->UpdateStats();
    }

    void TextChunk::Normalize() {
        std::size_t lines = 0;
        if (this->begin && !this->begin->Empty()) {
            this->begin->first_line = 0;
            lines = this->begin->GetLastLine() + 1;
        }
        if (this->end) {
            if (this->content.has_value()) {
                lines += this->newline.Count(this->content.value());
            }
            this->end->first_line = lines;
        }
    }

    void TextChunk::Remap() {
        this->chunk_map = std::make_unique<DefaultMap>(this->content, this->begin.get(), this->end.get(), this->newline);
    }

    void TextChunk::UpdateStats() {
        if (this->begin || this->end) {
            const std::size_t begin_height = this->begin ? this->begin->height : 0;
            const std::size_t end_height = this->end ? this->end->height : 0;
            this->height = std::max(begin_height, end_height) + 1;
        } else {
            this->height = 0;
        }
        this->has_content = (this->begin && this->begin->has_content) ||
            this->content.has_value() ||
            (this->end && this->end->has_content);
    }

    std::ostream &operator<<(std::ostream &os, const TextChunk &chunk) {
        if (chunk.begin && !chunk.begin->Empty()) {
            os << *chunk.begin;
        }
        if (chunk.content.has_value()) {
            if (chunk.begin && !chunk.begin->Empty()) {
                os << chunk.newline;
            }
            os << chunk.content.value();
        }
        if (chunk.end && !chunk.end->Empty()) {
            if ((chunk.begin && !chunk.begin->Empty()) || chunk.content.has_value()) {
                os << chunk.newline;
            }
            os << *chunk.end;
        }
        return os;
    }
}