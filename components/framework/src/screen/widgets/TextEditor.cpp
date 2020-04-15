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

#include "sloked/screen/widgets/TextEditor.h"

#include "sloked/core/Locale.h"
#include "sloked/sched/Pipeline.h"
#include "sloked/screen/TaggedFrame.h"
#include "sloked/services/Cursor.h"

namespace sloked {

    SlokedTextEditor::SlokedTextEditor(
        const Encoding &encoding, std::unique_ptr<KgrPipe> cursorService,
        std::function<void(SlokedCursorClient &)> initClient,
        std::unique_ptr<KgrPipe> renderService,
        std::unique_ptr<KgrPipe> notifyService,
        SlokedEditorDocumentSet::DocumentId docId, const std::string &tagger,
        SlokedBackgroundGraphics bg, SlokedForegroundGraphics fg)
        : conv(encoding, SlokedLocale::SystemEncoding()),
          cursorClient(std::move(cursorService)),
          renderClient(std::move(renderService), docId),
          notifyClient(std::move(notifyService), docId), tagger(tagger),
          background(bg), foreground(fg), cursorOffset{0, 0},
          renderCache([](const auto &, const auto &) -> std::vector<KgrValue> {
              throw SlokedError("TextEditor: Unexpected cache miss");
          }),
          lifetime(std::make_shared<SlokedStandardLifetime>()) {
        if (initClient) {
            initClient(this->cursorClient);
        }
        notifyClient.OnUpdate([this](const auto &evt) {
            if (this->updateListener) {
                this->updateListener();
            }
        });
    }

    SlokedTextEditor::~SlokedTextEditor() {
        this->lifetime->Close();
    }

    bool SlokedTextEditor::ProcessInput(const SlokedKeyboardInput &cmd) {
        if (cmd.value.index() == 0) {
            this->cursorClient.Insert(conv.Convert(std::get<0>(cmd.value)));
        } else
            switch (std::get<1>(cmd.value)) {
                case SlokedControlKey::ArrowUp:
                    this->cursorClient.MoveUp();
                    if (this->updateListener) {
                        this->updateListener();
                    }
                    break;

                case SlokedControlKey::ArrowDown:
                    this->cursorClient.MoveDown();
                    if (this->updateListener) {
                        this->updateListener();
                    }
                    break;

                case SlokedControlKey::ArrowLeft:
                    this->cursorClient.MoveBackward();
                    if (this->updateListener) {
                        this->updateListener();
                    }
                    break;

                case SlokedControlKey::ArrowRight:
                    this->cursorClient.MoveForward();
                    if (this->updateListener) {
                        this->updateListener();
                    }
                    break;

                case SlokedControlKey::Enter:
                    this->cursorClient.NewLine();
                    break;

                case SlokedControlKey::Tab:
                    this->cursorClient.Insert("\t");
                    break;

                case SlokedControlKey::Backspace:
                    this->cursorClient.DeleteBackward();
                    break;

                case SlokedControlKey::Delete:
                    this->cursorClient.DeleteForward();
                    break;

                case SlokedControlKey::Home:
                    this->cursorClient.Undo();
                    break;

                case SlokedControlKey::End:
                    this->cursorClient.Redo();
                    break;

                default:
                    return false;
            }
        return true;
    }

    TaskResult<void> SlokedTextEditor::RenderSurface(
        SlokedGraphicsPoint::Coordinate maxWidth, TextPosition::Line height,
        const SlokedFontProperties &fontProperties) {
        struct State {
            State(SlokedTextEditor *self,
                  SlokedGraphicsPoint::Coordinate maxWidth,
                  TextPosition::Line height,
                  const SlokedFontProperties &fontProperties)
                : self(self), maxWidth(maxWidth), height(height),
                  fontProperties(fontProperties), cursor{0, 0},
                  cursorOffset(self->cursorOffset), realPosition{0, 0} {}

            SlokedTextEditor *self;
            const SlokedGraphicsPoint::Coordinate maxWidth;
            const TextPosition::Line height;
            const SlokedFontProperties &fontProperties;

            TextPosition cursor;
            TextPosition cursorOffset;
            TextPosition realPosition;
            std::vector<SlokedTaggedTextFrame<bool>::TaggedLine> rendered;
        };

        static const SlokedAsyncTaskPipeline Pipeline(
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state,
                   std::optional<TextPosition> cursor) {
                    if (!cursor.has_value()) {
                        throw SlokedTaskPipelineStages::Cancel{};
                    }
                    state->cursor = cursor.value();
                    if (state->cursorOffset.line + state->height - 1 <
                        state->cursor.line) {
                        state->cursorOffset.line =
                            state->cursor.line - state->height + 1;
                    }
                    if (state->cursor.line < state->cursorOffset.line) {
                        state->cursorOffset.line = state->cursor.line;
                    }
                    return state->self->renderClient.PartialRender(
                        state->cursorOffset.line, state->height - 1);
                }),
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state,
                   std::tuple<
                       TextPosition::Line, TextPosition::Line,
                       std::vector<std::pair<TextPosition::Line, KgrValue>>>
                       result) {
                    state->self->renderCache.Insert(std::get<2>(result).begin(),
                                                    std::get<2>(result).end());
                    const auto &render = state->self->renderCache.Fetch(
                        std::get<0>(result), std::get<1>(result));
                    state->rendered.clear();
                    state->rendered.reserve(state->height);
                    for (auto it = render.first; it != render.second; ++it) {
                        SlokedTaggedTextFrame<bool>::TaggedLine taggedLine;
                        const auto &fragments = it->second.AsArray();
                        for (const auto &fragment : fragments) {
                            auto tag =
                                fragment.AsDictionary()["tag"].AsBoolean();
                            const auto &text =
                                fragment.AsDictionary()["content"].AsString();
                            taggedLine.fragments.push_back({tag, text});
                        }
                        state->rendered.emplace_back(std::move(taggedLine));
                    }
                    return state->self->renderClient.RealPosition(
                        state->cursor);
                }),
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state,
                   std::optional<TextPosition> realPosition) {
                    if (!realPosition.has_value()) {
                        throw SlokedTaskPipelineStages::Cancel{};
                    }
                    state->realPosition = realPosition.value();
                    SlokedTaggedTextFrame<bool> visualFrame(
                        state->self->conv.GetDestination(),
                        state->fontProperties);
                    auto currentLineOffset =
                        state->cursor.line - state->cursorOffset.line;
                    if (currentLineOffset < state->rendered.size()) {
                        const auto &currentLine =
                            state->rendered.at(currentLineOffset);
                        while (state->cursorOffset.column +
                                   visualFrame.GetMaxLength(
                                       currentLine, state->cursorOffset.column,
                                       state->maxWidth) <
                               realPosition.value().column) {
                            state->cursorOffset.column++;
                        }
                        if (realPosition.value().column <
                            state->cursorOffset.column) {
                            state->cursorOffset.column =
                                realPosition.value().column;
                        }

                        state->self->cursorOffset =
                            std::move(state->cursorOffset);
                        state->self->realPosition =
                            std::move(state->realPosition);
                        state->self->rendered = std::move(state->rendered);
                    }
                }));
        return Pipeline(
            std::make_shared<State>(this, maxWidth, height, fontProperties),
            this->cursorClient.GetPosition(), this->lifetime);
    }

    void SlokedTextEditor::ShowSurface(SlokedTextPane &pane) {
        pane.SetGraphicsMode(SlokedTextGraphics::Off);
        pane.SetGraphicsMode(this->background);
        pane.SetGraphicsMode(this->foreground);
        pane.ClearScreen();
        pane.SetPosition(0, 0);

        TextPosition::Line lineIdx{0};
        SlokedTaggedTextFrame<bool> visualFrame(this->conv.GetDestination(),
                                                pane.GetFontProperties());
        for (const auto &line : this->rendered) {
            pane.SetPosition(lineIdx++, 0);
            auto result = visualFrame.Slice(line, this->cursorOffset.column,
                                            pane.GetMaxWidth());
            for (const auto &fragment : result.fragments) {
                if (fragment.tag) {
                    pane.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
                } else {
                    pane.SetGraphicsMode(SlokedTextGraphics::Off);
                    pane.SetGraphicsMode(this->background);
                    pane.SetGraphicsMode(this->foreground);
                }
                pane.Write(this->conv.ReverseConvert(fragment.content));
            }
        }

        pane.SetPosition(this->realPosition.line - this->cursorOffset.line,
                         this->realPosition.column - this->cursorOffset.column);
    }

    void SlokedTextEditor::OnUpdate(std::function<void()> listener) {
        this->updateListener = listener;
    }
}  // namespace sloked