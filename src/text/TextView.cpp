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

#include "sloked/text/TextView.h"
#include "sloked/text/TextRegion.h"
#include "sloked/text/TextChunk.h"
#include "sloked/text/TextBlockHandle.h"
#include <map>
#include <iostream>

namespace sloked {


    std::unique_ptr<TextBlock> TextView::Open(std::string_view str, const NewLine &newline, const TextBlockFactory &blockFactory) {
        constexpr std::size_t MAX_CHUNK = 65536 * 32;
        std::unique_ptr<TextRegion> content = nullptr;
        std::size_t chunk_offset = 0;
        std::size_t last_line_offset = 0;
        std::size_t line = 0;
        std::map<std::size_t, std::pair<std::size_t, std::size_t>> chunk_lines;

        newline.Iterate(str, [&](std::size_t i, std::size_t width) {
            std::size_t length = i - chunk_offset;
            if (length < MAX_CHUNK) {
                chunk_lines[chunk_lines.size()] = std::make_pair(last_line_offset - chunk_offset, i - last_line_offset);
                last_line_offset = i + width;
            } else {
                auto proxyBlock = std::make_unique<TextBlockHandle>(str.substr(chunk_offset, last_line_offset > chunk_offset
                        ? last_line_offset - chunk_offset - width
                        : 0), std::move(chunk_lines), blockFactory);
                auto region = std::make_unique<TextRegion>(newline, std::move(proxyBlock));
                if (content) {
                    content->AppendRegion(std::move(region));
                } else {
                    content = std::move(region);
                }
                chunk_offset = last_line_offset;
                chunk_lines.clear();
                chunk_lines[chunk_lines.size()] = std::make_pair(last_line_offset - chunk_offset, i - last_line_offset);
                last_line_offset = i + width;
            }
            line++;
        });
        chunk_lines[chunk_lines.size()] = std::make_pair(last_line_offset - chunk_offset, str.size() - last_line_offset);
        
        if (chunk_offset != str.size()) {
            auto proxyBlock = std::make_unique<TextBlockHandle>(str.substr(chunk_offset), std::move(chunk_lines), blockFactory);
            auto region = std::make_unique<TextRegion>(newline, std::move(proxyBlock));
            if (content) {
                content->AppendRegion(std::move(region));
            } else {
                content = std::move(region);
            }
        }
        return content;
    }
}