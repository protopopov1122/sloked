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
            std::size_t offset;
            std::size_t length;
        };

        struct TaggedLine {
            std::string content;
            std::list<TaggedFragment> fragments;
        };

        SlokedTaggedTextFrame(const Encoding &encoding, const SlokedGraphemeEnumerator &graphemeIter,
                              const SlokedFontProperties &fontProperties)
            : encoding(encoding), graphemeIter(graphemeIter), fontProperties(fontProperties) {}

        TaggedLine Slice(const TaggedLine &input, TextPosition::Column virtualOffset,
                         SlokedGraphicsPoint::Coordinate maxWidth) {
            TaggedLine result;
            SlokedGraphicsPoint::Coordinate graphicalWidth{0};
            for (const auto &fragment : input.fragments) {
                if (graphicalWidth >= maxWidth) {
                    break;
                }

                auto view = std::string_view{input.content}.substr(fragment.offset, fragment.length);
                const auto fragmentBegin = result.content.size();
                this->graphemeIter.Iterate(this->encoding, view, [&](auto start, auto length, auto value) {
                    if (virtualOffset > 0) {
                        virtualOffset--;
                        return true;
                    }
                    result.content.append(view.substr(start, length));
                    graphicalWidth += this->fontProperties.GetWidth(value);
                    return graphicalWidth < maxWidth;
                });
                const auto fragmentLength = result.content.size() - fragmentBegin;
                if (fragmentLength > 0) {
                    result.fragments.push_back(TaggedFragment {
                        fragment.tag,
                        fragmentBegin,
                        fragmentLength
                    });
                }
            }
            return result;
        }

     private:
        const Encoding &encoding;
        const SlokedGraphemeEnumerator &graphemeIter;
        const SlokedFontProperties &fontProperties;
    };
}  // namespace sloked

#endif