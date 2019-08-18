#include "sloked/screen/widgets/TextEditor.h"
#include <sstream>
#include <algorithm>
#include <vector>

namespace sloked {

    SlokedTextEditor::SlokedTextEditor(TextBlock &text, SlokedCursor &cursor, SlokedTransactionJournal &journal, const EncodingConverter &conv, const SlokedCharWidth &charWidth, SlokedBackgroundGraphics bg)
        : text(text), cursor(cursor), journal(journal), conv(conv), charWidth(charWidth), frame(text, conv.GetSource(), charWidth), background(bg) {}

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

        pane.SetGraphicsMode(this->background);
        pane.ClearScreen();
        pane.SetPosition(0, 0);

        std::stringstream ss;
        ss << this->frame << std::endl;
        pane.Write(ss.str());
        
        auto realColumn = this->charWidth.GetRealPosition(std::string {this->text.GetLine(this->cursor.GetLine())}, this->cursor.GetColumn(), this->conv.GetSource());
        const auto &offset = this->frame.GetOffset();
        pane.SetPosition(this->cursor.GetLine() - offset.line, realColumn - offset.column);
    }
}