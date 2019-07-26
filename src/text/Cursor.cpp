#include "sloked/text/Cursor.h"

namespace sloked {

    SlokedCursor::SlokedCursor(TextBlock &text, const Encoding &encoding)
        : text(text), encoding(encoding), line(0), column(0) {
        this->UpdateCursor();
    }

    unsigned int SlokedCursor::GetLine() const {
        return this->line;
    }

    unsigned int SlokedCursor::GetColumn() const {
        return this->column;
    }

    void SlokedCursor::MoveUp(unsigned int l) {
        this->line -= std::min(l, this->line);
        this->UpdateCursor();
        this->column = std::min(this->column, this->width);
    }

    void SlokedCursor::MoveDown(unsigned int l) {
        this->line += std::min(this->line + l, static_cast<unsigned int>(this->text.GetLastLine())) - this->line;
        this->UpdateCursor();
        this->column = std::min(this->column, this->width);
    }

    void SlokedCursor::MoveForward(unsigned int c) {
        this->column = std::min(this->column + c, this->width);
        this->UpdateCursor();
    }

    void SlokedCursor::MoveBackward(unsigned int c) {
        this->column -= std::min(c, this->column);
        this->UpdateCursor();
    }

    void SlokedCursor::Insert(const std::string &frag) {
        std::string current {this->text.GetLine(this->line)};
        auto pos = this->encoding.GetCodepoint(current, this->column - 1);
        current.insert(pos.first + pos.second, frag);
        this->column += this->encoding.CodepointCount(frag);
        this->text.SetLine(this->line, current);
    }

    void SlokedCursor::NewLine() {
        std::string current {this->text.GetLine(this->line)};
        auto pos = this->encoding.GetCodepoint(current, this->column - 1);
        auto sub1 = current.substr(0, pos.first + pos.second);
        auto sub2 = current.substr(pos.first + pos.second);

        text.SetLine(line, sub1);
        text.InsertLine(line, sub2);
        this->line++;
        this->column = 0;
        this->UpdateCursor();
    }

    void SlokedCursor::Remove() {
        if (this->column > 0) {
            std::string current {this->text.GetLine(this->line)};
            auto pos1 = this->encoding.GetCodepoint(current, this->column - 2);
            auto pos2 = this->encoding.GetCodepoint(current, this->column - 1);
            auto sub1 = current.substr(0, pos1.first + pos1.second);
            auto sub2 = current.substr(pos2.first + pos2.second);

            text.SetLine(this->line, sub1 + sub2);
            this->column--;
            this->UpdateCursor();
        } else if (this->line > 0) {
            std::string ln1{text.GetLine(this->line - 1)};
            std::string ln2{text.GetLine(this->line)};
            text.SetLine(this->line - 1, ln1 + ln2);
            text.EraseLine(this->line);
            this->line--;

            this->column = this->encoding.CodepointCount(ln1);
            this->UpdateCursor();
        }
    }

    void SlokedCursor::UpdateCursor() {
        if (this->line <= this->text.GetLastLine()) {
            this->width = this->encoding.CodepointCount(std::string {this->text.GetLine(this->line)});
        } else {
            this->width = 0;
        }
    }
}