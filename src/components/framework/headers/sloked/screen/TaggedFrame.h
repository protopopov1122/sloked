/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_SCREEN_TAGGEDFRAME_H_
#define SLOKED_SCREEN_TAGGEDFRAME_H_

#include <functional>
#include <list>
#include <string>
#include <vector>

#include "sloked/core/Encoding.h"
#include "sloked/core/Grapheme.h"
#include "sloked/core/Position.h"
#include "sloked/screen/Character.h"
#include "sloked/screen/Point.h"

namespace sloked {

    template <typename Tag>
    class SlokedTaggedTextFrame {
     public:
        struct TaggedFragment {
            Tag tag;
            std::string content;
        };

        struct TaggedLine {
            std::list<TaggedFragment> fragments;
        };

        SlokedTaggedTextFrame(const Encoding &encoding, const SlokedGraphemeEnumerator &graphemeIter,
                              const SlokedFontProperties &fontProperties)
            : encoding(encoding), graphemeIter(graphemeIter), fontProperties(fontProperties) {}

        TaggedLine Slice(const TaggedLine &input, TextPosition::Column offset,
                         SlokedGraphicsPoint::Coordinate width) {
            TaggedLine result;
            TextPosition::Column currentOffset{0};
            SlokedGraphicsPoint::Coordinate currentWidth{0};
            for (const auto &fragment : input.fragments) {
                if (currentWidth >= width) {
                    break;
                }

                auto fragmentLength =
                    this->encoding.CodepointCount(fragment.content);
                auto [processed, processedWidth, processedLength] =
                    this->Process(
                        fragment.content,
                        currentOffset < offset ? offset - currentOffset : 0,
                        width - currentWidth);
                if (processedWidth > 0) {
                    currentWidth += processedWidth;
                    result.fragments.push_back({fragment.tag, std::string{processed}});
                }
                currentOffset += fragmentLength;
            }
            return result;
        }

        TextPosition::Column GetMaxLength(
            const TaggedLine &input, TextPosition::Column offset,
            SlokedGraphicsPoint::Coordinate width) {
            TextPosition::Column length{0}, currentOffset{0};
            SlokedGraphicsPoint::Coordinate currentWidth{0};
            for (const auto &fragment : input.fragments) {
                if (currentWidth >= width) {
                    break;
                }
                auto fragmentLength =
                    this->encoding.CodepointCount(fragment.content);
                auto [processed, processedWidth, processedLength] =
                    this->Process(
                        fragment.content,
                        currentOffset < offset ? offset - currentOffset : 0,
                        width - currentWidth);
                if (processedWidth > 0) {
                    currentWidth += processedWidth;
                    length += processedLength;
                }
                currentOffset += fragmentLength;
            }
            return length;
        }

     private:
        std::tuple<std::string_view, SlokedGraphicsPoint::Coordinate,
                   TextPosition::Column>
            Process(std::string_view src, std::size_t skip,
                    SlokedGraphicsPoint::Coordinate maxWidth) {
            std::string_view result = src;
            SlokedGraphicsPoint::Coordinate width{0};
            TextPosition::Column totalLength{0};
            std::size_t totalLengthBytes{0};

            this->graphemeIter.Iterate(this->encoding, src, [&](auto start, auto length,
                                                      auto grapheme) {
                if (skip > 0) {
                    result.remove_prefix(length);
                    skip--;
                    return true;
                }
                auto graphemeWidth = this->fontProperties.GetWidth(grapheme);
                if (width + graphemeWidth <= maxWidth) {
                    width += graphemeWidth;
                    totalLength++;
                    totalLengthBytes += length;
                    return true;
                } else {
                    return false;
                }
            });
            result.remove_suffix(result.size() - totalLengthBytes);
            return {result, width, totalLength};
        }

        const Encoding &encoding;
        const SlokedGraphemeEnumerator &graphemeIter;
        const SlokedFontProperties &fontProperties;
    };
}  // namespace sloked

#endif