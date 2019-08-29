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

#include "sloked/text/cursor/EditingPrimitives.h"

namespace sloked {

    TextPosition SlokedEditingPrimitives::Insert(TextBlock &text, const Encoding &encoding, const TextPosition &position, std::string_view content) {
        std::string current {text.GetLine(position.line)};
        if (position.column < encoding.CodepointCount(current)) {
            auto pos = encoding.GetCodepoint(current, position.column);
            current.insert(pos.first, content);
        } else {
            current.append(content);;
        }
        TextPosition result {position.line, static_cast<TextPosition::Column>(position.column + encoding.CodepointCount(content))};
        text.SetLine(position.line, current);
        return result;
    }

    TextPosition SlokedEditingPrimitives::Newline(TextBlock &text, const Encoding &encoding, const TextPosition &position, std::string_view content) {
        std::string current {text.GetLine(position.line)};
        if (position.column < encoding.CodepointCount(current)) {
            auto pos = encoding.GetCodepoint(current, position.column);
            auto sub1 = current.substr(0, pos.first);
            auto sub2 = current.substr(pos.first);

            text.SetLine(position.line, sub1);
            if (content.empty()) {
                text.InsertLine(position.line, sub2);
            } else {
                std::string str {content};
                str.append(sub2);
                text.InsertLine(position.line, str);
            }
        } else {
            text.InsertLine(position.line, content);
        }
        return TextPosition{position.line + 1, 0};
    }

    TextPosition SlokedEditingPrimitives::DeleteBackward(TextBlock &text, const Encoding &encoding, const TextPosition &position) {
        if (position.column > 0) {
            if (position.column < encoding.CodepointCount(text.GetLine(position.line))) {
                std::string current {text.GetLine(position.line)};
                auto pos1 = encoding.GetCodepoint(current, position.column - 1);
                auto pos2 = encoding.GetCodepoint(current, position.column);
                auto sub1 = current.substr(0, pos1.first);
                auto sub2 = current.substr(pos2.first);

                text.SetLine(position.line, sub1 + sub2);
            } else {
                auto current = text.GetLine(position.line);
                auto pos = encoding.GetCodepoint(current, position.column - 1);
                text.SetLine(position.line, current.substr(0, pos.first));
            }
            return TextPosition{position.line, position.column - 1};
        } else if (position.line > 0) {
            std::string ln1{text.GetLine(position.line - 1)};
            std::string ln2{text.GetLine(position.line)};
            text.SetLine(position.line - 1, ln1 + ln2);
            text.EraseLine(position.line);
            return TextPosition{position.line - 1, static_cast<TextPosition::Column>(encoding.CodepointCount(ln1))};
        } else {
            return position;
        }
    }

    TextPosition SlokedEditingPrimitives::DeleteForward(TextBlock &text, const Encoding &encoding, const TextPosition &position) {
        auto width = encoding.CodepointCount(text.GetLine(position.line));
        if (position.column + 1 < width) {
            std::string current {text.GetLine(position.line)};
            auto pos1 = encoding.GetCodepoint(current, position.column);
            auto pos2 = encoding.GetCodepoint(current, position.column + 1);
            auto sub1 = current.substr(0, pos1.first);
            auto sub2 = current.substr(pos2.first);
            text.SetLine(position.line, sub1 + sub2);
        } else if (position.column + 1 == width) {
            std::string current {text.GetLine(position.line)};
            auto pos = encoding.GetCodepoint(current, position.column);
            auto sub = current.substr(0, pos.first);
            text.SetLine(position.line, sub);
        } else if (position.column == width && position.line < text.GetLastLine()) {
            std::string ln1{text.GetLine(position.line)};
            std::string ln2{text.GetLine(position.line + 1)};
            text.SetLine(position.line, ln1 + ln2);
            text.EraseLine(position.line + 1);
        }
        return position;
    }

    TextPosition SlokedEditingPrimitives::ClearRegion(TextBlock &text, const Encoding &encoding, const TextPosition &from, const TextPosition &to) {
        if (!(from < to)) {
            return from;
        }
        if (from.line == to.line) {
            if (from.line <= text.GetLastLine()) {
                std::string line {text.GetLine(from.line)};
                std::size_t from_offset = SlokedEditingPrimitives::GetOffset(line, from.column, encoding);
                std::size_t to_offset = SlokedEditingPrimitives::GetOffset(line, to.column, encoding);
                text.SetLine(from.line, line.substr(0, from_offset) + line.substr(to_offset, line.size() - to_offset));
            }
            return from;
        }
        
        if (from.line <= text.GetLastLine()) {
            std::string first_line {text.GetLine(from.line)};
            std::string last_line = "";
            if (to.line <= text.GetLastLine()) {
                last_line = text.GetLine(to.line);
                text.EraseLine(to.line);
            }
            std::size_t from_offset = SlokedEditingPrimitives::GetOffset(first_line, from.column, encoding);
            std::size_t to_offset = SlokedEditingPrimitives::GetOffset(last_line, to.column, encoding);
            text.SetLine(from.line, first_line.substr(0, from_offset) + last_line.substr(to_offset, last_line.size() - to_offset));
        }

        for (std::size_t line = to.line - 1; line >= from.line + 1; line--) {
            if (line <= text.GetLastLine()) {
                text.EraseLine(line);
            }
        }
        return from;
    }

    std::size_t SlokedEditingPrimitives::GetOffset(std::string_view str, TextPosition::Column position, const Encoding &encoding) {
        std::size_t total_length = encoding.CodepointCount(str);
        if (position < total_length) {
            return encoding.GetCodepoint(str, position).first;
        } else {
            return str.size();
        }
    }

    std::vector<std::string> SlokedEditingPrimitives::Read(const TextBlock &text, const Encoding &encoding, const TextPosition &from, const TextPosition &to) {
        std::vector<std::string> result;
        if (from.line == to.line) {
            if (from.line <= text.GetLastLine()) {
                std::string line {text.GetLine(from.line)};
                std::size_t from_offset = SlokedEditingPrimitives::GetOffset(line, from.column, encoding);
                std::size_t to_offset = SlokedEditingPrimitives::GetOffset(line, to.column, encoding);
                result.push_back(line.substr(from_offset, to_offset - from_offset));
            }
            return result;
        }

        TextPosition::Line last_line = std::min(to.line, static_cast<TextPosition::Line>(text.GetLastLine()));
        for (TextPosition::Line line = from.line; line <= last_line; line++) {
            if (line == from.line) {
                auto first_line = text.GetLine(line);
                std::size_t from_offset = SlokedEditingPrimitives::GetOffset(first_line, from.column, encoding);
                result.push_back(std::string {first_line.substr(from_offset, first_line.size() - from_offset)});
            } else if (line == to.line) {
                auto last_line = text.GetLine(line);
                std::size_t to_offset = SlokedEditingPrimitives::GetOffset(last_line, to.column, encoding);
                result.push_back(std::string {last_line.substr(0, to_offset)});
            } else {
                result.push_back(std::string {text.GetLine(line)});
            }
        }
        return result;
    }
}