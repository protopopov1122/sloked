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

    SlokedTextEditor::SlokedTextEditor(const Encoding &encoding, std::unique_ptr<KgrPipe> cursorService, std::unique_ptr<KgrPipe> renderService, SlokedBackgroundGraphics bg)
        : conv(encoding, SlokedLocale::SystemEncoding()), cursorService(std::move(cursorService)), renderService(std::move(renderService)), background(bg) {}

    bool SlokedTextEditor::ProcessInput(const SlokedKeyboardInput &cmd) {
        if (cmd.index() == 0) {
            this->cursorService->Write(KgrDictionary {
                { "command", static_cast<int>(SlokedCursorService::Command::Insert) },
                { "content", KgrValue(conv.Convert(std::get<0>(cmd))) }
            });
        } else switch (std::get<1>(cmd)) {
            case SlokedControlKey::ArrowUp:
                this->cursorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedCursorService::Command::MoveUp) }
                });
                break;
            
            case SlokedControlKey::ArrowDown:
                this->cursorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedCursorService::Command::MoveDown) }
                });
                break;
            
            case SlokedControlKey::ArrowLeft:
                this->cursorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedCursorService::Command::MoveBackward) }
                });
                break;
            
            case SlokedControlKey::ArrowRight:
                this->cursorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedCursorService::Command::MoveForward) }
                });
                break;

            case SlokedControlKey::Enter:
                this->cursorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedCursorService::Command::NewLine) }
                });
                break;

            case SlokedControlKey::Tab:
                this->cursorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedCursorService::Command::Insert) },
                    { "content", KgrValue("\t") }
                });
                break;

            case SlokedControlKey::Backspace:
                this->cursorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedCursorService::Command::DeleteBackward) }
                });
                break;

            case SlokedControlKey::Delete:
                this->cursorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedCursorService::Command::DeleteForward) }
                });
                break;
            
            case SlokedControlKey::Escape:
                this->cursorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedCursorService::Command::Undo) }
                });
                break;
            
            case SlokedControlKey::End:
                this->cursorService->Write(KgrDictionary {
                    { "command", static_cast<int>(SlokedCursorService::Command::Redo) }
                });
                break;

            default:
                return false;
        }
        return true;
    }

    void SlokedTextEditor::Render(SlokedTextPane &pane) {
        this->cursorService->Write(KgrDictionary {
            { "command", static_cast<int>(SlokedCursorService::Command::Info) }
        });
        auto cursor = this->cursorService->ReadWait();
        this->renderService->Write(KgrDictionary {
            {
                "dim", KgrDictionary {
                    { "height", static_cast<int64_t>(pane.GetHeight()) },
                    { "width", static_cast<int64_t>(pane.GetWidth()) },
                    { "line", static_cast<int64_t>(cursor.AsDictionary()["line"].AsInt()) },
                    { "column", static_cast<int64_t>(cursor.AsDictionary()["column"].AsInt()) }
                }
            }
        });
        if (this->renderService->Count() > 1) {
            this->renderService->Drop(this->renderService->Count() - 1);
        }
        auto res = this->renderService->ReadWait();

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