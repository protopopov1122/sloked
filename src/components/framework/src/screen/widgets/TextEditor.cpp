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
#include "sloked/screen/widgets/TextEditorHelpers.h"

#include "sloked/core/Locale.h"
#include "sloked/sched/Pipeline.h"
#include "sloked/screen/TaggedFrame.h"
#include "sloked/services/Cursor.h"

namespace sloked {

    SlokedTextEditor::SlokedTextEditor(
        const Encoding &encoding, const SlokedCharPreset &charPreset,
        std::unique_ptr<KgrPipe> cursorService,
        std::function<void(SlokedCursorClient &)> initClient,
        std::unique_ptr<KgrPipe> renderService,
        std::unique_ptr<KgrPipe> notifyService,
        SlokedEditorDocumentSet::DocumentId docId, const std::string &tagger,
        SlokedBackgroundGraphics bg, SlokedForegroundGraphics fg)
        : conv(encoding, SlokedLocale::SystemEncoding()),
          charPreset(charPreset), cursorClient(std::move(cursorService)),
          renderClient(std::move(renderService), docId),
          notifyClient(std::move(notifyService), docId), tagger(tagger),
          background(bg),
          foreground(fg), virtualCursorOffset{0, 0}, virtualCursor{0, 0},
          renderCache(
              [](const auto &, const auto &)
                  -> std::vector<SlokedTaggedTextFrame<bool>::TaggedLine> {
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

        static const SlokedAsyncTaskPipeline Pipeline(
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<SlokedTextEditor::RenderingState> &state,
                   std::optional<TextPosition> cursor) {
                    if (!cursor.has_value()) {
                        throw SlokedTaskPipelineStages::Cancel{};
                    }
                    return state->RequestRender(cursor.value());
                }),
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<SlokedTextEditor::RenderingState> &state,
                   std::tuple<
                       TextPosition::Line, TextPosition::Line,
                       std::vector<std::pair<TextPosition::Line, KgrValue>>>
                       result) {
                    auto updatedTaggedLines = Helpers::ExtractTaggedLines(
                        std::get<2>(result).begin(), std::get<2>(result).end());
                    SlokedGraphemeNullEnumerator graphemeIter;
                    state->Render(updatedTaggedLines.begin(),
                                  updatedTaggedLines.end(), std::get<0>(result),
                                  std::get<1>(result), graphemeIter);

                    state->UpdateVirtualCursor(graphemeIter);
                    state->AdjustVirtualOffsetColumn(graphemeIter);
                    state->SaveResult();
                }),
            SlokedTaskPipelineStages::Catch(
                [](const std::shared_ptr<SlokedTextEditor::RenderingState> &,
                   const std::exception_ptr &) {
                    // Skip
                }),
            SlokedTaskPipelineStages::MapCancelled(
                [](const std::shared_ptr<SlokedTextEditor::RenderingState> &) {
                    // Skip
                }));
        return Pipeline(std::make_shared<SlokedTextEditor::RenderingState>(
                            this, maxWidth, height, fontProperties),
                        this->cursorClient.GetPosition(), this->lifetime);
    }

    void SlokedTextEditor::ShowSurface(SlokedTextPane &pane) {
        pane.SetGraphicsMode(SlokedTextGraphics::Off);
        pane.SetGraphicsMode(this->background);
        pane.SetGraphicsMode(this->foreground);
        pane.ClearScreen();
        pane.SetPosition(0, 0);

        TextPosition::Line lineIdx{0};
        SlokedGraphemeNullEnumerator graphemeIter;
        SlokedTaggedTextFrame<bool> visualFrame(this->conv.GetDestination(),
                                                graphemeIter,
                                                pane.GetFontProperties());
        for (const auto &line : this->rendered) {
            pane.SetPosition(lineIdx++, 0);
            auto result = visualFrame.Slice(
                line, this->virtualCursorOffset.column, pane.GetMaxWidth());
            for (const auto &fragment : result.fragments) {
                if (fragment.tag) {
                    pane.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
                } else {
                    pane.SetGraphicsMode(SlokedTextGraphics::Off);
                    pane.SetGraphicsMode(this->background);
                    pane.SetGraphicsMode(this->foreground);
                }

                auto view = std::string_view{result.content}.substr(
                    fragment.offset, fragment.length);
                pane.Write(this->conv.ReverseConvert(view));
            }
        }

        pane.SetPosition(
            this->virtualCursor.line - this->virtualCursorOffset.line,
            this->virtualCursor.column - this->virtualCursorOffset.column);
    }

    void SlokedTextEditor::OnUpdate(std::function<void()> listener) {
        this->updateListener = listener;
    }
}  // namespace sloked