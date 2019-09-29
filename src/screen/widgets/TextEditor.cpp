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
#include "sloked/services/TextEditor.h"
#include "sloked/core/Locale.h"

namespace sloked {

    SlokedTextEditor::SlokedTextEditor(const Encoding &encoding, std::unique_ptr<KgrPipe> editorService, SlokedBackgroundGraphics bg)
        : conv(encoding, SlokedLocale::SystemEncoding()), editorService(std::move(editorService)), background(bg) {}

    bool SlokedTextEditor::ProcessInput(const SlokedKeyboardInput &cmd) {
        if (cmd.index() == 0) {
            this->editorService->Write(KgrDictionary {
                { "command", static_cast<int>(SlokedTextEditorService::Command::Insert) },
                { "content", KgrValue(conv.Convert(std::get<0>(cmd))) }
            });
        } else switch (std::get<1>(cmd)) {
            case SlokedControlKey::ArrowUp:
                this->editorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedTextEditorService::Command::MoveUp) }
                });
                break;
            
            case SlokedControlKey::ArrowDown:
                this->editorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedTextEditorService::Command::MoveDown) }
                });
                break;
            
            case SlokedControlKey::ArrowLeft:
                this->editorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedTextEditorService::Command::MoveBackward) }
                });
                break;
            
            case SlokedControlKey::ArrowRight:
                this->editorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedTextEditorService::Command::MoveForward) }
                });
                break;

            case SlokedControlKey::Enter:
                this->editorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedTextEditorService::Command::NewLine) }
                });
                break;

            case SlokedControlKey::Tab:
                this->editorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedTextEditorService::Command::Insert) },
                    { "content", KgrValue("\t") }
                });
                break;

            case SlokedControlKey::Backspace:
                this->editorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedTextEditorService::Command::DeleteBackward) }
                });
                break;

            case SlokedControlKey::Delete:
                this->editorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedTextEditorService::Command::DeleteForward) }
                });
                break;
            
            case SlokedControlKey::Escape:
                this->editorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedTextEditorService::Command::Undo) }
                });
                break;
            
            case SlokedControlKey::End:
                this->editorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedTextEditorService::Command::Redo) }
                });
                break;

            default:
                return false;
        }
        return true;
    }

    void SlokedTextEditor::Render(SlokedTextPane &pane) {
        this->editorService->Write(KgrDictionary {
            { "command", static_cast<int>(SlokedTextEditorService::Command::Render) },
            {
                "dim", KgrDictionary {
                    { "height", static_cast<int64_t>(pane.GetHeight()) },
                    { "width", static_cast<int64_t>(pane.GetWidth()) }
                }
            }
        });
        if (this->editorService->Count() > 1) {
            this->editorService->Drop(this->editorService->Count() - 1);
        }
        auto res = this->editorService->ReadWait();

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
        
        const auto &cursor = res.AsDictionary()["cursor"].AsDictionary();
        pane.SetPosition(cursor["line"].AsInt(), cursor["column"].AsInt());
    }
}