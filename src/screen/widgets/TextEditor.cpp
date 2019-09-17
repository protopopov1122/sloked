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
#include <sstream>
#include <algorithm>
#include <vector>
#include <iostream>

namespace sloked {

    class TextCharWidthIterator {
     public:
        TextCharWidthIterator(std::string_view line, TextPosition::Column offset, const Encoding &encoding, const SlokedCharWidth &charWidth)
            : line(line), column(0), offset(offset), encoding(encoding), charWidth(charWidth) {
            this->line_codepoints = this->encoding.CodepointCount(this->line);
            this->line_real_length = this->charWidth.GetRealPosition(this->line, this->line_codepoints, this->encoding).second;
            if  (this->offset < this->line_real_length) {
                this->SeekColumn();
            }
        }

        TextPosition::Column GetRealColumn() const {
            return this->column;
        }

        void Next() {
            if (this->offset++ < this->line_real_length) {
                this->SeekColumn();
            }
        }

     private:
        void SeekColumn() {
            auto realPos = this->charWidth.GetRealPosition(this->line, this->column + 1, this->encoding);
            while (realPos.first <= this->offset && this->column + 1 < this->line_codepoints) {
                realPos = this->charWidth.GetRealPosition(this->line, ++this->column + 1, this->encoding);
            }
        }

        std::string line;
        std::size_t line_codepoints;
        std::size_t line_real_length;
        TextPosition::Column column;
        TextPosition::Column offset;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
    };

    SlokedTextEditor::SlokedTextEditor(TextBlock &text, SlokedCursor &cursor, SlokedTransactionJournal &journal, SlokedTaggedText<int> &tags,
        const EncodingConverter &conv, const SlokedCharWidth &charWidth, SlokedBackgroundGraphics bg)
        : text(text), cursor(cursor), journal(journal), tags(tags), conv(conv), charWidth(charWidth),
          frame(text, conv.GetSource(), charWidth), background(bg) {}

    bool SlokedTextEditor::ProcessInput(const SlokedKeyboardInput &cmd) {
        if (cmd.index() == 0) {
            this->cursor.Insert(conv.ReverseConvert(std::get<0>(cmd)));
        } else switch (std::get<1>(cmd)) {
            case SlokedControlKey::ArrowUp:
                cursor.MoveUp(1);
                break;
            
            case SlokedControlKey::ArrowDown:
                cursor.MoveDown(1);
                break;
            
            case SlokedControlKey::ArrowLeft:
                cursor.MoveBackward(1);
                break;
            
            case SlokedControlKey::ArrowRight:
                cursor.MoveForward(1);
                break;

            case SlokedControlKey::Enter:
                cursor.NewLine("");
                break;

            case SlokedControlKey::Tab:
                cursor.Insert(conv.GetSource().Encode(U'\t'));
                break;

            case SlokedControlKey::Backspace:
                cursor.DeleteBackward();
                break;

            case SlokedControlKey::Delete:
                cursor.DeleteForward();
                break;
            
            case SlokedControlKey::Escape:
                journal.Undo();
                break;
            
            case SlokedControlKey::End:
                journal.Redo();
                break;

            default:
                return false;
        }
        return true;
    }

    void SlokedTextEditor::Render(SlokedTextPane &pane) {
        this->frame.Update(TextPosition{pane.GetHeight(), pane.GetWidth()}, TextPosition{this->cursor.GetLine(), this->cursor.GetColumn()});

        pane.SetGraphicsMode(this->background);
        pane.ClearScreen();
        pane.SetPosition(0, 0);

        TextPosition::Line lineNumber = 0;
        this->frame.Visit(0, std::min(pane.GetHeight(), static_cast<TextPosition::Line>(this->frame.GetLastLine()) + 1), [&](const auto lineView) {
            std::string line{lineView};
            std::string fullLine{this->text.GetLine(this->frame.GetOffset().line + lineNumber)};

            const auto lineLength = this->conv.GetSource().CodepointCount(line);
            std::size_t frameOffset = this->frame.GetOffset().column;
            
            TextCharWidthIterator iter(fullLine, frameOffset, this->conv.GetSource(), this->charWidth);
            for (TextPosition::Column column = 0; column < lineLength; column++, iter.Next()) {
                auto tag = this->tags.Get(TextPosition{lineNumber, iter.GetRealColumn()});
                if (tag) {
                    pane.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
                } else {
                    pane.SetGraphicsMode(SlokedTextGraphics::Off);
                    pane.SetGraphicsMode(this->background);
                }
                auto pos = this->conv.GetSource().GetCodepoint(line, column);
                auto fragment = this->conv.Convert(line.substr(pos.first, pos.second));
                pane.Write(fragment);
            }
            if (lineNumber++ < this->frame.GetLastLine()) {
                pane.Write(this->conv.GetDestination().Encode(U"\n"));
            }
        });
        
        std::string realLine{this->text.GetLine(this->cursor.GetLine())};
        auto realPos = this->charWidth.GetRealPosition(realLine, this->cursor.GetColumn(), this->conv.GetSource());
        auto realColumn = this->cursor.GetColumn() < this->conv.GetSource().CodepointCount(realLine) ? realPos.first : realPos.second;
        const auto &offset = this->frame.GetOffset();
        pane.SetPosition(this->cursor.GetLine() - offset.line, realColumn - offset.column);
    }
}