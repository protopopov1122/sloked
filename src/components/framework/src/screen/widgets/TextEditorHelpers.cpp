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

#include "sloked/screen/widgets/TextEditorHelpers.h"

namespace sloked {

    SlokedTaggedTextFrame<bool>::TaggedLine
        SlokedTextEditor::Helpers::ExtractTaggedLine(const KgrValue &raw) {
        SlokedTaggedTextFrame<bool>::TaggedLine taggedLine;
        const auto &fragments = raw.AsArray();
        for (const auto &fragment : fragments) {
            auto tag = fragment.AsDictionary()["tag"].AsBoolean();
            const auto &text = fragment.AsDictionary()["content"].AsString();
            const auto fragmentBegin = taggedLine.content.size();
            taggedLine.content.append(text);
            taggedLine.fragments.push_back({tag, fragmentBegin, text.size()});
        }
        return taggedLine;
    }

    void SlokedTextEditor::Helpers::UnwrapFragment(
        std::string_view in, const Encoding &encoding,
        const SlokedCharPreset &charPreset,
        const SlokedGraphemeEnumerator &graphemes, TextPosition::Column &column,
        std::string &out) {
        graphemes.Iterate(
            encoding, in, [&](auto start, auto length, auto codepoints) {
                if (codepoints.Size() == 1 && codepoints[0] == U'\t') {
                    out.append(SlokedCharPreset::EncodeTab(charPreset, encoding,
                                                           column));
                    column += charPreset.GetCharWidth(U'\t', column);
                } else {
                    out.append(in.substr(start, length));
                    column++;
                }
                return true;
            });
    }

    SlokedTaggedTextFrame<bool>::TaggedLine
        SlokedTextEditor::Helpers::UnwrapTaggedLine(
            const SlokedTaggedTextFrame<bool>::TaggedLine &line,
            const Encoding &encoding, const SlokedCharPreset &charPreset,
            const SlokedGraphemeEnumerator &graphemes) {
        std::string_view view{line.content};
        SlokedTaggedTextFrame<bool>::TaggedLine taggedLine;
        TextPosition::Column column{0};
        for (const auto &fragment : line.fragments) {
            const auto fragmentBegin = taggedLine.content.size();
            UnwrapFragment(view.substr(fragment.offset, fragment.length),
                           encoding, charPreset, graphemes, column,
                           taggedLine.content);
            taggedLine.fragments.push_back(
                {fragment.tag, fragmentBegin,
                 taggedLine.content.size() - fragmentBegin});
        }
        return taggedLine;
    }

    SlokedTextEditor::RenderingState::RenderingState(
        SlokedTextEditor *self, SlokedGraphicsPoint::Coordinate maxWidth,
        TextPosition::Line height, const SlokedFontProperties &fontProperties)
        : self(self), maxWidth(maxWidth), height(height),
          fontProperties(fontProperties), directCursor{0, 0}, virtualCursor{0,
                                                                            0},
          virtualCursorOffset(self->virtualCursorOffset) {}

    SlokedTextEditor::RenderingState::RenderResult
        SlokedTextEditor::RenderingState::RequestRender(
            const TextPosition &cursor) {
        this->directCursor = cursor;
        return this->self->renderClient.PartialRender(
            this->UpdateVirtualOffsetLine(), this->height - 1);
    }

    void SlokedTextEditor::RenderingState::AdjustVirtualOffsetColumn(
        const SlokedGraphemeEnumerator &graphemes) {
        const auto &currentTaggedLine =
            this->self->renderCache.Get(this->virtualCursor.line);
        std::vector<GraphemeBounds> graphemeList;
        GraphemeBounds::Split(
            currentTaggedLine.content, this->self->conv.GetDestination(),
            this->self->charPreset, graphemes, this->fontProperties,
            std::back_inserter(graphemeList));
        if (this->virtualCursorOffset.column > this->virtualCursor.column) {
            this->virtualCursorOffset.column = this->virtualCursor.column;
        }
        while (GraphemeBounds::TotalGraphicalWidth(
                   graphemeList.begin(), graphemeList.end(),
                   this->virtualCursorOffset.column,
                   this->virtualCursor.column) > this->maxWidth) {
            this->virtualCursorOffset.column++;
        }
    }

    void SlokedTextEditor::RenderingState::UpdateVirtualCursor(
        const SlokedGraphemeEnumerator &graphemes) {
        this->virtualCursor.line = this->directCursor.line;
        const auto &currentTaggedLine =
            this->self->renderCache.Get(this->virtualCursor.line);
        std::vector<GraphemeBounds> graphemeList;
        GraphemeBounds::Split(
            currentTaggedLine.content, this->self->conv.GetDestination(),
            this->self->charPreset, graphemes, this->fontProperties,
            std::back_inserter(graphemeList));
        const auto currentGrapheme = GraphemeBounds::FindByDirectOffset(
            graphemeList.begin(), graphemeList.end(),
            this->directCursor.column);
        this->virtualCursor.column =
            currentGrapheme != graphemeList.end()
                ? currentGrapheme->virtualOffset
                : (graphemeList.empty()
                       ? 0
                       : graphemeList.back().virtualOffset + 1);
    }

    void SlokedTextEditor::RenderingState::SaveResult() {
        this->self->virtualCursorOffset = std::move(this->virtualCursorOffset);
        this->self->virtualCursor = std::move(this->virtualCursor);
        this->self->rendered = std::move(this->rendered);
    }

    TextPosition::Line
        SlokedTextEditor::RenderingState::UpdateVirtualOffsetLine() {
        if (this->virtualCursorOffset.line + this->height - 1 <
            this->directCursor.line) {
            this->virtualCursorOffset.line =
                this->directCursor.line - this->height + 1;
        }
        if (this->directCursor.line < this->virtualCursorOffset.line) {
            this->virtualCursorOffset.line = this->directCursor.line;
        }
        return this->virtualCursorOffset.line;
    }
}  // namespace sloked