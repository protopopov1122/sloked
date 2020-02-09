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

#include "sloked/screen/widgets/TextEditor.h"
#include "sloked/services/Cursor.h"
#include "sloked/core/Locale.h"
#include "sloked/screen/TaggedFrame.h"

namespace sloked {

    SlokedTextEditor::SlokedTextEditor(const Encoding &encoding, std::unique_ptr<KgrPipe> cursorService, std::function<void(SlokedCursorClient &)> initClient,
        std::unique_ptr<KgrPipe> renderService, std::unique_ptr<KgrPipe> notifyService, SlokedEditorDocumentSet::DocumentId docId, const std::string &tagger, SlokedBackgroundGraphics bg)
        : conv(encoding, SlokedLocale::SystemEncoding()), cursorClient(std::move(cursorService)), renderClient(std::move(renderService), docId), notifyClient(std::move(notifyService), docId), tagger(tagger), background(bg),
          cursorOffset{0, 0} {
        if (initClient) {
            initClient(this->cursorClient);
        }
        notifyClient.OnUpdate([this](const auto &evt) {
            if (this->updateListener) {
                this->updateListener();
            }
        });
    }

    bool SlokedTextEditor::ProcessInput(const SlokedKeyboardInput &cmd) {
        if (cmd.value.index() == 0) {
            this->cursorClient.Insert(conv.Convert(std::get<0>(cmd.value)));
        } else switch (std::get<1>(cmd.value)) {
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

    void SlokedTextEditor::Render(SlokedTextPane &pane) {
        auto cursorRsp = this->cursorClient.GetPosition();
        if (!cursorRsp.has_value()) {
            return;
        }
        const auto &cursor = cursorRsp.value();
        if (this->cursorOffset.line + pane.GetHeight() - 1 < cursor.line) {
            this->cursorOffset.line = cursor.line - pane.GetHeight() + 1;
        }
        if (cursor.line < this->cursorOffset.line) {
            this->cursorOffset.line = cursor.line;
        }

        auto renderRsp = this->renderClient.Render(this->cursorOffset.line, pane.GetHeight());
        if (!renderRsp.has_value()) {
            return;
        }
        const auto &render = renderRsp.value();
        std::vector<SlokedTaggedTextFrame<bool>::TaggedLine> lines;
        lines.reserve(pane.GetHeight());
        for (const auto &line : render.AsArray()) {
            SlokedTaggedTextFrame<bool>::TaggedLine taggedLine;
            const auto &fragments = line.AsArray();
            for (const auto &fragment : fragments) {
                auto tag = fragment.AsDictionary()["tag"].AsBoolean();
                const auto &text = fragment.AsDictionary()["content"].AsString();
                taggedLine.fragments.push_back({tag, text});
            }
            lines.emplace_back(std::move(taggedLine));
        }

        SlokedTaggedTextFrame<bool> visualFrame(this->conv.GetDestination(), pane.GetFontProperties());

        auto realPosition = this->renderClient.RealPosition(cursor);
        if (!realPosition.has_value()) {
            return;
        }
        auto currentLineOffset = cursor.line - this->cursorOffset.line;
        if (currentLineOffset < lines.size()) {
            const auto &currentLine = lines.at(currentLineOffset);
            while (this->cursorOffset.column + visualFrame.GetMaxLength(currentLine, this->cursorOffset.column, pane.GetMaxWidth()) < realPosition.value().column) {
                this->cursorOffset.column++;
            }
            if (realPosition.value().column < this->cursorOffset.column) {
                this->cursorOffset.column = realPosition.value().column;
            }
        }

        pane.SetGraphicsMode(this->background);
        pane.ClearScreen();
        pane.SetPosition(0, 0);

        TextPosition::Line lineIdx{0};
        for (const auto &line : lines) {
            pane.SetPosition(lineIdx++, 0);
            auto result = visualFrame.Slice(line, this->cursorOffset.column, pane.GetMaxWidth());
            for (const auto &fragment : result.fragments) {
                if (fragment.tag) {
                    pane.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
                } else {
                    pane.SetGraphicsMode(SlokedTextGraphics::Off);
                    pane.SetGraphicsMode(this->background);
                }
                pane.Write(this->conv.ReverseConvert(fragment.content));
            }
        }
        
        pane.SetPosition(cursor.line - this->cursorOffset.line, realPosition.value().column - this->cursorOffset.column);
    }

    void SlokedTextEditor::OnUpdate(std::function<void()> listener) {
        this->updateListener = listener;
    }
}