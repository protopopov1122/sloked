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

namespace sloked {

    SlokedTextEditor::SlokedTextEditor(const Encoding &encoding, std::unique_ptr<KgrPipe> cursorService, std::function<void(SlokedCursorClient &)> initClient,
        std::unique_ptr<KgrPipe> renderService, SlokedEditorDocumentSet::DocumentId docId, const std::string &tagger, SlokedBackgroundGraphics bg)
        : conv(encoding, SlokedLocale::SystemEncoding()), cursorClient(std::move(cursorService)), notifyClient(std::move(renderService), docId), tagger(tagger), background(bg) {
        if (initClient) {
            initClient(this->cursorClient);
        }
        notifyClient.OnUpdate([this] {
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
        auto renderRes = this->cursorClient.Render(TextPosition {
            pane.GetHeight(),
            pane.GetWidth() - 1
        }, this->tagger);
        if (!renderRes.has_value()) {
            return;
        }
        const auto &res = renderRes.value().first;

        pane.SetGraphicsMode(this->background);
        pane.ClearScreen();
        pane.SetPosition(0, 0);

        const auto &fragments = res.AsDictionary()["content"].AsArray();
        for (const auto &fragment : fragments) {
            auto tag = fragment.AsDictionary()["tag"].AsBoolean();
            const auto &text = fragment.AsDictionary()["content"].AsString();
            if (tag) {
                pane.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
            } else {
                pane.SetGraphicsMode(SlokedTextGraphics::Off);
                pane.SetGraphicsMode(this->background);
            }
            pane.Write(text);
        }
        
        const auto &realCursor = res.AsDictionary()["cursor"].AsDictionary();
        pane.SetPosition(realCursor["line"].AsInt(), realCursor["column"].AsInt());
    }

    void SlokedTextEditor::OnUpdate(std::function<void()> listener) {
        this->updateListener = listener;
    }
}