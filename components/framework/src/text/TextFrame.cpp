/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

    TextFrameView::TextFrameView(const TextBlockView &text, const Encoding &encoding, const SlokedCharPreset &charPreset)
        : text(text), encoding(encoding), charPreset(charPreset), offset{0, 0}, size{0, 0}, buffer{} {}

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
            return this->buffer[idx] ? this->buffer[idx]->content : "";
        } else {
            return "";
        }
    }

    bool TextFrameView::Empty() const {
        return this->GetTotalLength() == 0;
    }

    bool TextFrameView::Update(const TextPosition &dim, const TextPosition &cursor, bool force) {
        auto prevPosition = this->offset;
        auto prevSize = this->size;
        if (this->offset.line + dim.line - 1 < cursor.line) {
            this->offset.line = cursor.line - dim.line + 1;
        }
        if (cursor.line < offset.line) {
            this->offset.line = cursor.line;
        }

        auto realColumn = this->charPreset.GetRealPosition(std::string {this->text.GetLine(cursor.line)}, cursor.column, this->encoding).first;
        if (this->offset.column + dim.column - 1 < realColumn) {
            this->offset.column = realColumn - dim.column + 1;
        }
        if (realColumn < this->offset.column) {
            this->offset.column = realColumn;
        }
        this->size = dim;
        if (force || prevPosition != this->offset || prevSize != this->size) {
            if (!force && prevSize == this->size && prevPosition.column == this->offset.column) {
                this->OptimizedRender(prevPosition);
            } else {
                this->FullRender();
            }
            return true;
        } else {
            return false;
        }
    }

    const TextPosition &TextFrameView::GetOffset() const {
        return this->offset;
    }

    const TextPosition &TextFrameView::GetSize() const {
        return this->size;
    }

    void TextFrameView::VisitSymbols(std::function<void(std::size_t, std::size_t, const std::vector<std::string_view> &)> callback) const {
        std::size_t counter = this->offset.line;
        for (const auto &line : this->buffer) {
            if (line) {
                callback(counter++, line->leftOffset, line->symbols);
            } else {
                callback(counter++, 0, {});
            }
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

    void TextFrameView::FullRender() {
        this->buffer.clear();
        this->text.Visit(this->offset.line, std::min(this->size.line, static_cast<TextPosition::Line>(this->text.GetLastLine() - this->offset.line) + 1), [&](const auto line) {
            this->buffer.push_back(this->CutLine(line));
        });
    }

    void TextFrameView::OptimizedRender(const TextPosition &prevOffset) {
        auto start = this->offset.line;
        std::size_t lineId = 0;
        auto callback = [&](const auto line) {
            this->buffer.insert(this->buffer.begin() + lineId++, this->CutLine(line));
        };
        if (prevOffset.line < this->offset.line) {
            auto count = start - prevOffset.line;
            this->buffer.erase(this->buffer.begin(), this->buffer.begin() + count);
            lineId = this->buffer.size();
            this->text.Visit(start + this->size.line - count, count, callback);
        } else {
            auto count = prevOffset.line - start;
            this->buffer.erase(this->buffer.end() - count, this->buffer.end());
            lineId = 0;
            this->text.Visit(start, count, callback);
        }
    }

    void TextFrameView::VisitLines(std::function<void(std::size_t, std::string_view)> callback) const {
        std::size_t lineId = 0;
        this->text.Visit(this->offset.line, std::min(this->size.line, static_cast<TextPosition::Line>(this->text.GetLastLine() - this->offset.line) + 1), [&](const auto line) {
            if (this->buffer[lineId]) {
                callback(lineId, this->buffer[lineId]->content);
            } else {
                callback(lineId, "");
            }
            lineId++;
        });
    }

    std::unique_ptr<TextFrameView::Line> TextFrameView::CutLine(std::string_view line) const {
        std::list<std::string_view> content;
        auto result = std::make_unique<Line>(Line {
            {},
            std::string{this->charPreset.GetTab(this->encoding)},
            "",
            0
        });

        std::string preprocessedLine;
        this->encoding.IterateCodepoints(line, [&](auto start, auto length, auto chr) {
            if (chr != u'\t') {
                content.push_back(line.substr(start, length));
                preprocessedLine.append(line.substr(start, length));
            } else {
                content.push_back(result->tab);
                preprocessedLine.append(result->tab);
            }
            return true;
        });
        auto leftOffset = this->encoding.GetCodepoint(preprocessedLine, this->offset.column);
        auto rightOffset = this->encoding.GetCodepoint(preprocessedLine, this->offset.column + this->size.column);
        auto leftIter = content.begin();
        auto rightIter = content.end();
        if (leftOffset.second != 0) {
            TextPosition::Column currentPosition = 0;
            for (auto it = content.begin(); it != content.end();) {
                const auto &symbol = *it;
                auto length = symbol.size();
                if (currentPosition + length < leftOffset.first) {
                    result->leftOffset++;
                    leftIter = ++it;
                } else if (currentPosition < leftOffset.first && currentPosition + length >= leftOffset.first) {
                    auto cut = leftOffset.first - currentPosition;
                    *it = it->substr(cut, it->size() - cut);
                    ++it;
                } else if (currentPosition >= leftOffset.first && (currentPosition + length < rightOffset.first || rightOffset.second == 0)) {
                    ++it;
                } else if (currentPosition < rightOffset.first && currentPosition + length >= rightOffset.first) {
                    auto cut = currentPosition + length - rightOffset.first;
                    *it = it->substr(cut, it->size() - cut);
                    ++it;
                } else {
                    rightIter = it;
                    break;
                }
                currentPosition += length;
            }

            result->symbols.insert(result->symbols.end(), leftIter, rightIter);
            result->content = preprocessedLine.substr(leftOffset.first, rightOffset.first - leftOffset.first);
            return result;
        } else {
            return nullptr;
        }
    }
}