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

#ifndef SLOKED_SCREEN_WIDGETS_TEXT_EDITOR_HELPERS_H_
#define SLOKED_SCREEN_WIDGETS_TEXT_EDITOR_HELPERS_H_

#include <algorithm>

#include "sloked/core/CharPreset.h"
#include "sloked/core/Encoding.h"
#include "sloked/core/Grapheme.h"
#include "sloked/core/Position.h"
#include "sloked/screen/Character.h"
#include "sloked/screen/widgets/TextEditor.h"

namespace sloked {

    struct GraphemeBounds {
        struct Position {
            TextPosition::Column offset;
            TextPosition::Column length;
        };

        template <typename Iter>
        static Iter FindByDirectOffset(const Iter &begin, const Iter &end,
                                       TextPosition::Column directOffset) {
            return std::find_if(
                begin, end, [directOffset](const auto &grapheme) {
                    return directOffset >= grapheme.directPosition.offset &&
                           directOffset < grapheme.directPosition.offset +
                                              grapheme.directPosition.length;
                });
        }

        template <typename OutIter>
        static void Split(std::string_view in, const Encoding &encoding,
                          const SlokedCharPreset &charPreset,
                          const SlokedGraphemeEnumerator &graphemes,
                          const SlokedFontProperties &fontProperties,
                          OutIter out) {
            TextPosition::Column column{0};
            TextPosition::Column codepointOffset{0};
            graphemes.Iterate(
                encoding, in, [&](auto start, auto length, auto codepoints) {
                    if (codepoints.Size() == 1 && codepoints[0] == U'\t') {
                        auto tabWidth = charPreset.GetCharWidth(U'\t', column);
                        const char32_t Space{U' '};
                        while (tabWidth-- > 0) {
                            *out++ = GraphemeBounds{
                                column, codepointOffset,
                                static_cast<TextPosition::Column>(
                                    codepoints.Size()),
                                fontProperties.GetWidth(SlokedSpan(&Space, 1))};
                            column++;
                        }
                    } else {
                        *out++ =
                            GraphemeBounds{column, codepointOffset,
                                           static_cast<TextPosition::Column>(
                                               codepoints.Size()),
                                           fontProperties.GetWidth(codepoints)};
                        column++;
                    }
                    codepointOffset += codepoints.Size();
                    return true;
                });
        }

        template <typename Iter>
        static auto TotalGraphicalWidth(const Iter &begin, const Iter &end,
                                        TextPosition::Column from,
                                        TextPosition::Column to) {
            SlokedGraphicsPoint::Coordinate total{0};
            std::for_each(begin, end, [from, to, &total](const auto &grapheme) {
                if (from <= grapheme.virtualOffset &&
                    grapheme.virtualOffset <= to) {
                    total += grapheme.graphicalWidth;
                }
            });
            return total;
        }

        TextPosition::Column virtualOffset;
        Position directPosition;
        SlokedGraphicsPoint::Coordinate graphicalWidth;
    };

    class SlokedTextEditor::Helpers {
     public:
        static SlokedTaggedTextFrame<bool>::TaggedLine ExtractTaggedLine(
            const KgrValue &);

        static void UnwrapFragment(std::string_view, const Encoding &,
                                   const SlokedCharPreset &,
                                   const SlokedGraphemeEnumerator &,
                                   TextPosition::Column &, std::string &);

        static SlokedTaggedTextFrame<bool>::TaggedLine UnwrapTaggedLine(
            const SlokedTaggedTextFrame<bool>::TaggedLine &, const Encoding &,
            const SlokedCharPreset &, const SlokedGraphemeEnumerator &);

        template <typename Iter>
        static auto ExtractTaggedLines(const Iter &begin, const Iter &end) {
            std::vector<std::pair<TextPosition::Line,
                                  SlokedTaggedTextFrame<bool>::TaggedLine>>
                result;
            for (auto it = begin; it != end; ++it) {
                result.emplace_back(
                    std::make_pair(it->first, ExtractTaggedLine(it->second)));
            }
            return result;
        }
    };

    class SlokedTextEditor::RenderingState {
     public:
        using RenderResult = TaskResult<
            std::tuple<TextPosition::Line, TextPosition::Line,
                       std::vector<std::pair<TextPosition::Line, KgrValue>>>>;

        RenderingState(SlokedTextEditor *, SlokedGraphicsPoint::Coordinate,
                       TextPosition::Line, const SlokedFontProperties &);
        RenderResult RequestRender(const TextPosition &);
        void AdjustVirtualOffsetColumn(const SlokedGraphemeEnumerator &);
        void UpdateVirtualCursor(const SlokedGraphemeEnumerator &);
        void SaveResult();

        template <typename Iter>
        void Render(const Iter &begin, const Iter &end, TextPosition::Line from,
                    TextPosition::Line to,
                    const SlokedGraphemeEnumerator &graphemes) {
            this->self->renderCache.Insert(begin, end);
            const auto &render = this->self->renderCache.Fetch(from, to);
            this->rendered.clear();
            this->rendered.reserve(this->height);
            for (auto it = render.first; it != render.second; ++it) {
                this->rendered.emplace_back(Helpers::UnwrapTaggedLine(
                    it->second, this->self->conv.GetDestination(),
                    this->self->charPreset, graphemes));
            }
        }

     private:
        TextPosition::Line UpdateVirtualOffsetLine();

        SlokedTextEditor *self;
        const SlokedGraphicsPoint::Coordinate maxWidth;
        const TextPosition::Line height;
        const SlokedFontProperties &fontProperties;

        TextPosition directCursor;
        TextPosition virtualCursor;
        TextPosition virtualCursorOffset;
        std::vector<SlokedTaggedTextFrame<bool>::TaggedLine> rendered;
    };
}  // namespace sloked

#endif