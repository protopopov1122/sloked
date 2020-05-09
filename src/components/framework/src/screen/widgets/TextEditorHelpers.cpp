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

#include <iostream>

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

    SlokedTextEditor::RendererState::RendererState(
        SlokedTextEditor *self, SlokedGraphicsPoint::Coordinate maxWidth,
        TextPosition::Line height, const SlokedFontProperties &fontProperties)
        : self(self),
          model(self->documentState.DeriveRendererFrame(height, maxWidth)),
          directCursor{self->documentState.virtualCursor},
          fontProperties(fontProperties) {}

    SlokedTextEditor::RendererState::RenderResult
        SlokedTextEditor::RendererState::RequestRender(
            const TextPosition &cursor) {
        this->directCursor = cursor;
        return this->self->renderClient.PartialRender(
            this->model.CalculateVerticalOffset(this->directCursor.line),
            this->model.GetHeight() - 1);
    }

    void SlokedTextEditor::RendererState::UpdateCursor(
        const SlokedGraphemeEnumerator &graphemes) {
        const auto &currentTaggedLine =
            this->self->documentState.renderCache.Get(this->directCursor.line);
        SlokedGraphemeStringLayout lineLayout(
            currentTaggedLine.content, this->self->conv.GetDestination(),
            this->self->charPreset, graphemes, this->fontProperties);
        this->model.SetDirectCursor(this->directCursor, lineLayout);
    }

    void SlokedTextEditor::RendererState::SaveResult() {
        this->self->documentState.virtualCursorOffset =
            std::move(this->model.GetVirtualCursorOffset());
        this->self->documentState.virtualCursor =
            std::move(this->model.GetVirtualCursor());
        this->self->documentState.rendered = std::move(this->rendered);
    }

    SlokedTextEditor::RendererFrame::RendererFrame(
        TextPosition::Line height, SlokedGraphicsPoint::Coordinate maxWidth,
        const TextPosition &directCursor,
        const TextPosition &virtualCursorOffset,
        const TextPosition &virtualCursor)
        : height{height}, maxWidth{maxWidth}, directCursor{directCursor},
          virtualCursorOffset{virtualCursorOffset}, virtualCursor{
                                                        virtualCursor} {}

    TextPosition::Line SlokedTextEditor::RendererFrame::GetHeight() const {
        return this->height;
    }

    SlokedGraphicsPoint::Coordinate
        SlokedTextEditor::RendererFrame::GetMaxWidth() const {
        return this->maxWidth;
    }

    TextPosition::Line SlokedTextEditor::RendererFrame::CalculateVerticalOffset(
        TextPosition::Line line) const {
        TextPosition::Line offset = this->virtualCursorOffset.line;
        if (offset + this->height - 1 < line) {
            offset = line - this->height + 1;
        }
        if (line < offset) {
            offset = line;
        }
        return offset;
    }

    const TextPosition &SlokedTextEditor::RendererFrame::GetDirectCursor()
        const {
        return this->directCursor;
    }

    const TextPosition &SlokedTextEditor::RendererFrame::GetVirtualCursor()
        const {
        return this->virtualCursor;
    }

    const TextPosition &
        SlokedTextEditor::RendererFrame::GetVirtualCursorOffset() const {
        return this->virtualCursorOffset;
    }
}  // namespace sloked