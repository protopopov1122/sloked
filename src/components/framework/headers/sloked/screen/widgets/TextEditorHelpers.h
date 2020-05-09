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
#include "sloked/screen/GraphemeString.h"
#include "sloked/screen/widgets/TextEditor.h"

namespace sloked {

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

    class SlokedTextEditor::RendererFrame {
     public:
        using Tag = bool;
        RendererFrame(TextPosition::Line, SlokedGraphicsPoint::Coordinate,
                      const TextPosition &, const TextPosition &,
                      const TextPosition &);
        TextPosition::Line GetHeight() const;
        SlokedGraphicsPoint::Coordinate GetMaxWidth() const;
        TextPosition::Line CalculateVerticalOffset(TextPosition::Line) const;
        const TextPosition &GetDirectCursor() const;
        const TextPosition &GetVirtualCursor() const;
        const TextPosition &GetVirtualCursorOffset() const;

        void SetDirectCursor(const TextPosition &directCursor,
                             const SlokedGraphemeStringLayout &lineLayout) {
            this->directCursor = directCursor;
            this->UpdateVirtualCursor(lineLayout);
            this->UpdateVirtualCursorOffset(lineLayout);
        }

     private:
        void UpdateVirtualCursor(const SlokedGraphemeStringLayout &lineLayout) {
            this->virtualCursor.line = this->directCursor.line;
            const auto currentGrapheme =
                lineLayout.FindByDirectOffset(this->directCursor.column);
            this->virtualCursor.column =
                currentGrapheme != lineLayout.end()
                    ? currentGrapheme->virtualOffset
                    : (lineLayout.Count() == 0
                           ? 0
                           : std::prev(lineLayout.end())->virtualOffset + 1);
        }

        void UpdateVirtualCursorOffset(
            const SlokedGraphemeStringLayout &lineLayout) {
            this->virtualCursorOffset.line =
                this->CalculateVerticalOffset(this->virtualCursor.line);
            if (this->virtualCursorOffset.column > this->virtualCursor.column) {
                this->virtualCursorOffset.column = this->virtualCursor.column;
            }
            while (lineLayout.TotalGraphicalWidth(
                       lineLayout.begin() + this->virtualCursorOffset.column,
                       std::min(lineLayout.begin() + this->virtualCursor.column + 1,
                                lineLayout.end())) > this->maxWidth) {
                this->virtualCursorOffset.column++;
            }
        }

        TextPosition::Line height;
        SlokedGraphicsPoint::Coordinate maxWidth;
        TextPosition directCursor;
        TextPosition virtualCursorOffset;
        TextPosition virtualCursor;
    };

    class SlokedTextEditor::RendererState {
     public:
        using RenderResult = TaskResult<
            std::tuple<TextPosition::Line, TextPosition::Line,
                       std::vector<std::pair<TextPosition::Line, KgrValue>>>>;

        RendererState(SlokedTextEditor *, SlokedGraphicsPoint::Coordinate,
                      TextPosition::Line, const SlokedFontProperties &);
        RenderResult RequestRender(const TextPosition &);

        template <typename Iter>
        void Render(const Iter &begin, const Iter &end, TextPosition::Line from,
                    TextPosition::Line to,
                    const SlokedGraphemeEnumerator &graphemes) {
            this->self->documentState.renderCache.Insert(begin, end);
            const auto &render =
                this->self->documentState.renderCache.Fetch(from, to);
            this->rendered.clear();
            this->rendered.reserve(this->model.GetHeight());
            for (auto it = render.first; it != render.second; ++it) {
                this->rendered.emplace_back(Helpers::UnwrapTaggedLine(
                    it->second, this->self->conv.GetDestination(),
                    this->self->charPreset, graphemes));
            }
            this->UpdateCursor(graphemes);
            this->SaveResult();
        }

     private:
        void UpdateCursor(const SlokedGraphemeEnumerator &);
        void SaveResult();

        SlokedTextEditor *self;

        RendererFrame model;
        TextPosition directCursor;
        const SlokedFontProperties &fontProperties;
        std::vector<SlokedTaggedTextFrame<bool>::TaggedLine> rendered;
    };
}  // namespace sloked

#endif