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

#ifndef SLOKED_SCREEN_VISUALFRAME_H_
#define SLOKED_SCREEN_VISUALFRAME_H_

#include "sloked/core/Position.h"
#include "sloked/core/CharPreset.h"
#include "sloked/core/Encoding.h"
#include "sloked/screen/Point.h"
#include "sloked/screen/Character.h"
#include <list>
#include <vector>
#include <string>
#include <functional>

namespace sloked {

    template <typename Tag>
    class SlokedTextVisualFrame {
     public:
        struct TaggedFragment {
            Tag tag;
            std::string content;
        };

        struct TaggedLine {
            std::list<TaggedFragment> fragments;
        };

        SlokedTextVisualFrame(const Encoding &encoding, const SlokedCharPreset &charPreset, const SlokedCharacterVisualPreset &visualCharPreset)
            : encoding(encoding), charPreset(charPreset), visualCharPreset(visualCharPreset) {}

        TaggedLine CutLine(const TaggedLine &input, TextPosition::Column offset, SlokedGraphicsPoint::Coordinate width) {
            TaggedLine result;
            TextPosition::Column currentOffset{0};
            SlokedGraphicsPoint::Coordinate currentWidth{0};
            for (const auto &fragment : input.fragments) {
                if (currentWidth >= width) {
                    break;
                }
                
                auto length = this->encoding.CodepointCount(fragment.content);
                if (currentOffset + length < offset) {
                } else if (currentOffset < offset) {
                    auto [unwrapped, unwrappedWidth] = this->Unwrap(fragment.content, offset - currentOffset, width - currentWidth);
                    currentWidth += unwrappedWidth;
                    result.fragments.push_back({fragment.tag, std::move(unwrapped)});
                } else {
                    auto [unwrapped, unwrappedWidth] = this->Unwrap(fragment.content, 0, width - currentWidth);
                    currentWidth += unwrappedWidth;
                    result.fragments.push_back({fragment.tag, std::move(unwrapped)});
                }
                currentOffset += length;
            }
            return result;
        }

     private:
        std::pair<std::string, SlokedGraphicsPoint::Coordinate> Unwrap(std::string_view src, std::size_t skip, SlokedGraphicsPoint::Coordinate maxWidth) {
            std::string result;
            SlokedGraphicsPoint::Coordinate width{0};
            const auto tab = this->charPreset.GetTab(this->encoding);
            SlokedGraphicsPoint::Coordinate tabWidth{0};
            this->encoding.IterateCodepoints(tab, [&](auto start, auto length, auto value) {
                tabWidth += this->visualCharPreset.GetWidth(value);
                return true;
            });
            this->encoding.IterateCodepoints(src, [&](auto start, auto length, auto value) {
                if (skip > 0) {
                    skip--;
                    return true;
                }
                std::string cpContent;
                SlokedGraphicsPoint::Coordinate cpWidth{0};
                if (value != U'\t') {
                    cpContent = src.substr(start, length);
                    cpWidth = this->visualCharPreset.GetWidth(value);
                } else {
                    cpContent = tab;
                    cpWidth = tabWidth;
                }
                if (width + cpWidth <= maxWidth) {
                    result.append(cpContent);
                    width += cpWidth;
                    return true;
                } else {
                    return false;
                }
            });
            return { result, width };
        }

        const Encoding &encoding;
        const SlokedCharPreset &charPreset;
        const SlokedCharacterVisualPreset &visualCharPreset;
    };
}

#endif