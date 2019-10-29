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

#include "sloked/text/TextFrame.h"
#include <list>
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
            return this->buffer[idx].content;
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
        this->text.Visit(this->offset.line, std::min(this->size.line, static_cast<TextPosition::Line>(this->text.GetLastLine() - this->offset.line) + 1), [&](const auto line) {
            this->buffer.push_back(this->CutLine(line));
        });
    }

    const TextPosition &TextFrameView::GetOffset() const {
        return this->offset;
    }

    const TextPosition &TextFrameView::GetSize() const {
        return this->size;
    }

    void TextFrameView::VisitSymbols(std::function<void(std::size_t, std::size_t, const std::vector<std::pair<char32_t, std::string>> &)> callback) const {
        std::size_t counter = this->offset.line;
        for (const auto &line : this->buffer) {
            callback(counter++, line.leftOffset, line.symbols);
        }
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
            callback(lineId, this->buffer[lineId].content);
            lineId++;
        });
    }

    TextFrameView::Line TextFrameView::CutLine(std::string_view line) const {
        std::list<std::pair<char32_t, std::string>> content;
        std::stringstream fullLine;
        this->encoding.IterateCodepoints(line, [&](auto start, auto length, auto chr) {
            if (chr != u'\t') {
                content.push_back(std::make_pair(chr, std::string{line.substr(start, length)}));
                fullLine << line.substr(start, length);
            } else {
                content.push_back(std::make_pair(chr, this->charWidth.GetTab(this->encoding)));
                fullLine << this->charWidth.GetTab(this->encoding);
            }
            return true;
        });
        auto leftOffset = this->encoding.GetCodepoint(fullLine.str(), this->offset.column);
        auto rightOffset = this->encoding.GetCodepoint(fullLine.str(), this->offset.column + this->size.column);
        std::size_t leftSymbolOffset = 0;
        if (leftOffset.second != 0) {
            TextPosition::Column currentPosition = 0;
            for (auto it = content.begin(); it != content.end();) {
                const auto &symbol = *it;
                auto length = symbol.second.size();
                if (currentPosition + length < leftOffset.first) {
                    leftSymbolOffset++;
                    auto curIt = it++;
                    content.erase(curIt);
                } else if (currentPosition < leftOffset.first && currentPosition + length >= leftOffset.first) {
                    auto cut = leftOffset.first - currentPosition;
                    it->second = it->second.substr(cut, it->second.size() - cut);
                    ++it;
                } else if (currentPosition >= leftOffset.first && (currentPosition + length < rightOffset.first || rightOffset.second == 0)) {
                    ++it;
                } else if (currentPosition < rightOffset.first && currentPosition + length >= rightOffset.first) {
                    auto cut = currentPosition + length - rightOffset.first;
                    it->second = it->second.substr(cut, it->second.size() - cut);
                    ++it;
                } else {
                    content.erase(it, content.end());
                    break;
                }
                currentPosition += length;
            }

            fullLine.str("");
            for (const auto &symbol : content) {
                fullLine << symbol.second;
            }
            return {
                std::vector<std::pair<char32_t, std::string>>(content.begin(), content.end()),
                fullLine.str(),
                leftSymbolOffset
            };
        } else {
            return {{}, "", 0};
        }
    }
}