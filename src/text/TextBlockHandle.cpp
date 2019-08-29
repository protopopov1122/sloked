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

#include "sloked/text/TextBlockHandle.h"
#include <iostream>
#include <cassert>
#include "sloked/core/Error.h"

namespace sloked {


    TextBlockHandle::TextBlockHandle(std::string_view content, std::map<std::size_t, std::pair<std::size_t, std::size_t>> lines, const TextBlockFactory &factory)
        : content(view {content, std::move(lines)}), factory(factory) {}

    std::size_t TextBlockHandle::GetLastLine() const {
        switch (this->content.index()) {
            case 0:
                return std::get<0>(this->content).lines.size() - 1;
            case 1:
                return std::get<1>(this->content)->GetLastLine();
            default:
                assert(false);
        }
    }

    std::size_t TextBlockHandle::GetTotalLength() const {
        switch (this->content.index()) {
            case 0:
                return std::get<0>(this->content).content.size();
            case 1:
                return std::get<1>(this->content)->GetTotalLength();
            default:
                assert(false);
        }
    }

    std::string_view TextBlockHandle::GetLine(std::size_t line) const {
        switch (this->content.index()) {
            case 0: {
                const auto &content = std::get<0>(this->content);
                if (content.lines.count(line)) {
                    const auto &pos = content.lines.at(line);
                    return content.content.substr(pos.first, pos.second);
                } else {
                    throw SlokedError("Line " + std::to_string(line) + " exceeds total length of block");
                }
            }
            case 1:
                return std::get<1>(this->content)->GetLine(line);
            default:
                assert(false);
        }
    }

    bool TextBlockHandle::Empty() const {
        switch (this->content.index()) {
            case 0:
                return std::get<0>(this->content).lines.empty();
            case 1:
                return std::get<1>(this->content)->Empty();
            default:
                assert(false);
        }
    }

    void TextBlockHandle::Visit(std::size_t start, std::size_t count, Visitor visitor) const {
        switch (this->content.index()) {
            case 0: {
                const view &content = std::get<0>(this->content);
                for (std::size_t i = start; i < start + count; i++) {
                    if (content.lines.count(i)) {
                        const auto &pos = content.lines.at(i);
                        visitor(content.content.substr(pos.first, pos.second));
                    } else {
                        throw SlokedError("Line " + std::to_string(i) + " exceeds total length of block");   
                    }
                }
            } break;
            case 1:
                std::get<1>(this->content)->Visit(start, count, visitor);
                break;
            default:
                assert(false);
        }
    }

    void TextBlockHandle::SetLine(std::size_t line, std::string_view content) {
        this->open_block();
        std::get<1>(this->content)->SetLine(line, content);
    }

    void TextBlockHandle::EraseLine(std::size_t line) {
        this->open_block();
        std::get<1>(this->content)->EraseLine(line);
    }

    void TextBlockHandle::InsertLine(std::size_t line, std::string_view content) {
        this->open_block();
        std::get<1>(this->content)->InsertLine(line, content);
    }

    void TextBlockHandle::Optimize() {
        if (this->content.index() == 1) {
            std::get<1>(this->content)->Optimize();
        }
    }

    std::ostream &operator<<(std::ostream &os, const TextBlockHandle &block) {
        switch (block.content.index()) {
            case 0:
                return os << std::get<0>(block.content).content;
            case 1:
                return os << *std::get<1>(block.content);
            default:
                assert(false);
        }
    }

    void TextBlockHandle::open_block() const {
        if (this->content.index() == 0) {
            this->content = this->factory.make(std::get<0>(this->content).content);
        }
    }
}