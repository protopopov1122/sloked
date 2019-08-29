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

#include "sloked/text/TextFrame.h"
#include <iostream>
#include <sstream>

namespace sloked {

    TextFrameView::TextFrameView(const TextBlockView &text, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : text(text), encoding(encoding), charWidth(charWidth), offset{0, 0}, size{0, 0}, buffer{} {}

    std::size_t TextFrameView::TextFrameView::GetLastLine() const {
        return std::min(static_cast<std::size_t>(this->offset.line + this->size.line), this->text.GetLastLine()) - this->offset.line;
    }

    std::size_t TextFrameView::GetTotalLength() const {
        std::size_t length = 0;
        this->VisitLines([&](auto line, auto content) {
            length += content.size() + 1;
        });
        if (length > 0) {
            length--;
        }
        return length;
    }

    std::string_view TextFrameView::GetLine(std::size_t idx) const {
        if (this->offset.line + idx <= this->text.GetLastLine() && idx <= this->size.line) {
            std::string line {this->PreprocessLine(this->text.GetLine(this->offset.line + idx))};
            this->buffer[idx] = line;
            return this->buffer[idx];
        } else {
            return "";
        }
    }

    bool TextFrameView::Empty() const {
        return this->GetTotalLength() == 0;
    }

    void TextFrameView::Update(const TextPosition &dim, const TextPosition &cursor) {
        if (this->offset.line + dim.line - 1 < cursor.line) {
            this->offset.line = cursor.line - dim.line + 1;
        }
        if (cursor.line < offset.line) {
            this->offset.line = cursor.line;
        }

        auto realColumn = this->charWidth.GetRealPosition(std::string {this->text.GetLine(cursor.line)}, cursor.column, this->encoding).first;
        if (this->offset.column + dim.column - 1 < realColumn) {
            this->offset.column = realColumn - dim.column + 1;
        }
        if (realColumn < this->offset.column) {
            this->offset.column = realColumn;
        }
        this->size = dim;
        this->buffer.clear();
        this->buffer.insert(this->buffer.end(), this->size.line + 1, "");
    }

    const TextPosition &TextFrameView::GetOffset() const {
        return this->offset;
    }

    const TextPosition &TextFrameView::GetSize() const {
        return this->size;
    }

    std::ostream &TextFrameView::dump(std::ostream &os) const {
        auto lastLine = this->GetLastLine();
        this->VisitLines([&](auto line, auto content) {
            os << content;
            if (line < lastLine) {
                os << std::endl;
            }
        });
        return os;
    }

    void TextFrameView::VisitLines(std::function<void(std::size_t, std::string_view)> callback) const {
        std::size_t lineId = 0;
        this->text.Visit(this->offset.line, std::min(this->size.line, static_cast<TextPosition::Line>(this->text.GetLastLine() - this->offset.line) + 1), [&](const auto line) {
            callback(lineId++, this->PreprocessLine(line));
        });
    }

    std::string TextFrameView::PreprocessLine(std::string_view line) const {
        std::stringstream ss;
        this->encoding.IterateCodepoints(line, [&](auto start, auto length, auto chr) {
            if (chr != u'\t') {
                ss << line.substr(start, length);
            } else {
                ss << this->charWidth.GetTab();
            }
            return true;
        });
        auto offsetLine = this->encoding.GetCodepoint(ss.str(), this->offset.column);
        auto sizeLine = this->encoding.GetCodepoint(ss.str(), this->offset.column + this->size.column);
        if (offsetLine.second != 0) {
            return ss.str().substr(offsetLine.first,
                sizeLine.second != 0
                    ? sizeLine.first - offsetLine.first
                    : std::string::npos);
        } else {
            return "";
        }
    }
}