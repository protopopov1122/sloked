#include "sloked/screen/widgets/TextEditor.h"
#include <sstream>
#include <algorithm>
#include <vector>
#include <iostream>

namespace sloked {

    SlokedTextEditor::SlokedTextEditor(TextBlock &text, SlokedCursor &cursor, SlokedTransactionJournal &journal, SlokedTaggedText<int> &tags,
        const EncodingConverter &conv, const SlokedCharWidth &charWidth, SlokedBackgroundGraphics bg)
        : text(text), cursor(cursor), journal(journal), tags(tags), conv(conv), charWidth(charWidth),
          frame(text, conv.GetSource(), charWidth), tagsView(tags, TextPosition{0, 0}, TextPosition{0, 0}), background(bg) {}

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
                cursor.Insert(conv.GetSource().Encode(u'\t'));
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
        this->tagsView.Update(this->frame.GetOffset(), this->frame.GetSize());

        pane.SetGraphicsMode(this->background);
        pane.ClearScreen();
        pane.SetPosition(0, 0);

        TextPosition::Line lineNumber = 0;
        this->frame.Visit(0, std::min(pane.GetHeight(), static_cast<TextPosition::Line>(this->frame.GetLastLine()) + 1), [&](const auto lineView) {
            std::string line{lineView};
            const auto lineLength = this->conv.GetSource().CodepointCount(line);
            for (TextPosition::Column column = 0; column < lineLength; column++) {
                auto tag = tagsView.Get(TextPosition{lineNumber, column});
                if (tag) {
                    pane.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
                } else {
                    pane.SetGraphicsMode(SlokedTextGraphics::Off);
                    pane.SetGraphicsMode(this->background);
                }
                auto pos = this->conv.GetSource().GetCodepoint(line, column);
                pane.Write(line.substr(pos.first, pos.second));
            }
            if (lineNumber++ < this->frame.GetLastLine()) {
                pane.Write("\n");
            }
        });
        
        auto realColumn = this->charWidth.GetRealPosition(std::string {this->text.GetLine(this->cursor.GetLine())}, this->cursor.GetColumn(), this->conv.GetSource()).first;
        const auto &offset = this->frame.GetOffset();
        pane.SetPosition(this->cursor.GetLine() - offset.line, realColumn - offset.column);
    }
}