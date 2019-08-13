#include "sloked/screen/widgets/TextEditor.h"
#include <sstream>
#include <algorithm>
#include <vector>

namespace sloked {

    SlokedTextEditor::SlokedTextEditor(TextBlock &text, SlokedCursor &cursor, SlokedTransactionJournal &journal, const EncodingConverter &conv, const ScreenCharWidth &charWidth)
        : text(text), cursor(cursor), journal(journal), conv(conv), charWidth(charWidth), offset{0, 0} {}

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
        if (offset.line + pane.GetHeight() - 1 < cursor.GetLine()) {
            offset.line = cursor.GetLine() - pane.GetHeight() + 1;
        }
        if (cursor.GetLine() < offset.line) {
            offset.line = cursor.GetLine();
        }

        auto realColumn = this->charWidth.GetRealPosition(std::string {this->text.GetLine(cursor.GetLine())}, cursor.GetColumn(), this->conv.GetSource());
        if (offset.column + pane.GetWidth() - 1 < realColumn) {
            offset.column = realColumn - pane.GetWidth() + 1;
        }
        if (realColumn < offset.column) {
            offset.column = realColumn;
        }

        std::vector<std::string> lines;
        text.Visit(offset.line, std::min(pane.GetHeight(), static_cast<TextPosition::Line>(text.GetLastLine() - offset.line)), [&](const auto line) {
            std::stringstream ss;
            this->conv.GetSource().IterateCodepoints(line, [&](auto start, auto length, auto chr) {
                if (chr != u'\t') {
                    ss << line.substr(start, length);
                } else {
                    ss << this->charWidth.GetTab();
                }
                return true;
            });
            lines.push_back(ss.str());
        });

        pane.ClearScreen();
        pane.SetPosition(0, 0);

        for (const auto line : lines) {
            std::stringstream ss;
            auto offsetLine = this->conv.GetSource().GetCodepoint(line, offset.column);
            if (offsetLine.second != 0) {
                ss << line.substr(offsetLine.first);
            }
            ss << std::endl;
            pane.Write(ss.str());
        }
        pane.SetPosition(cursor.GetLine() - offset.line, realColumn - offset.column);
    }
}