/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

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

    SlokedTextEditor::SlokedTextEditor(const Encoding &encoding, std::unique_ptr<KgrPipe> cursorService, std::unique_ptr<KgrPipe> renderService, SlokedEditorDocumentSet::DocumentId docId, SlokedBackgroundGraphics bg)
        : conv(encoding, SlokedLocale::SystemEncoding()), cursorClient(std::move(cursorService)), renderClient(std::move(renderService), docId), background(bg) {
        this->cursorClient.Connect(docId);
    }

    bool SlokedTextEditor::ProcessInput(const SlokedKeyboardInput &cmd) {
        if (cmd.value.index() == 0) {
            this->cursorClient.Insert(conv.Convert(std::get<0>(cmd.value)));
        } else switch (std::get<1>(cmd.value)) {
            case SlokedControlKey::ArrowUp:
                this->cursorClient.MoveUp();
                break;
            
            case SlokedControlKey::ArrowDown:
                this->cursorClient.MoveDown();
                break;
            
            case SlokedControlKey::ArrowLeft:
                this->cursorClient.MoveBackward();
                break;
            
            case SlokedControlKey::ArrowRight:
                this->cursorClient.MoveForward();
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
            
            case SlokedControlKey::Escape:
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
        const auto &cursor = this->cursorClient.GetPosition();
        if (!cursor.has_value()) {
            return;
        }
        auto renderRes = this->renderClient.Render(cursor.value(), TextPosition {
            pane.GetHeight(),
            pane.GetWidth() - 1
        });
        if (!renderRes.has_value()) {
            return;
        }
        const auto &res = renderRes.value();

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
}