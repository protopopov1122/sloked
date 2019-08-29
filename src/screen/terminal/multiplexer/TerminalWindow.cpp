#include "sloked/screen/terminal/multiplexer/TerminalWindow.h"
#include "sloked/core/Encoding.h"
#include "sloked/core/CharWidth.h"
#include <iostream>

namespace sloked {

    TerminalWindow::TerminalWindow(SlokedTerminal &term, const Encoding &encoding, const SlokedCharWidth &charWidth, const TextPosition &offset, const TextPosition &size)
        : term(term), encoding(encoding), charWidth(charWidth), offset(offset), size(size), line(0), col(0) {}

    void TerminalWindow::Move(const TextPosition &position) {
        this->offset = position;
    }

    void TerminalWindow::Resize(const TextPosition &size) {
        this->size = size;
    }

    const TextPosition &TerminalWindow::GetOffset() const {
        return this->offset;
    }

    TerminalWindow TerminalWindow::SubWindow(const TextPosition &offset, const TextPosition &size) const {
        return TerminalWindow(this->term, this->encoding, this->charWidth,
            TextPosition{this->offset.line + offset.line, this->offset.column + offset.column},
            size);
    }

    void TerminalWindow::SetPosition(Line y, Column x) {
        this->line = std::min(y, this->size.line - 1);
        this->col = std::min(x, this->size.column - 1);
        this->term.SetPosition(this->offset.line + this->line, this->offset.column + this->col);
    }

    void TerminalWindow::MoveUp(Line l) {
        l = std::min(this->line, l);
        this->line -= l;
        this->term.MoveUp(l);
    }

    void TerminalWindow::MoveDown(Line l) {
        l = std::min(this->line + l, this->size.line - 1) - this->line;
        this->line += l;
        this->term.MoveDown(l);
    }

    void TerminalWindow::MoveBackward(Column c) {
        c = std::min(this->col, c);
        this->col -= c;
        this->term.MoveBackward(c);
    }

    void TerminalWindow::MoveForward(Column c) {
        c = std::min(this->col + c, this->size.column - 1) - this->col;
        this->col += c;
        this->term.MoveForward(c);
    }

    void TerminalWindow::ShowCursor(bool show) {
        this->term.ShowCursor(show);
    }

    void TerminalWindow::ClearScreen() {
        std::string cl(this->size.column, ' ');
        for (std::size_t y = this->offset.line; y < this->offset.line + this->size.line; y++) {
            this->term.SetPosition(y, this->offset.column);
            this->term.Write(cl);
        }
    }

    void TerminalWindow::ClearChars(Column c) {
        c = std::min(this->col + c, this->size.column - 1) - this->col;
        this->term.ClearChars(c);
    }

    TerminalWindow::Column TerminalWindow::GetWidth() {
        return this->size.column;
    }

    TerminalWindow::Line TerminalWindow::GetHeight() {
        return this->size.line;
    }

    void TerminalWindow::Write(const std::string &str) {
        std::string buffer;
        std::string_view view = str;
        this->encoding.IterateCodepoints(str, [&](auto start, auto length, auto codepoint) {
            if (this->line >= this->size.line || this->line + this->offset.line >= this->term.GetHeight()) {
                return false;
            }
            if (codepoint == U'\n') {
                this->term.Write(buffer);
                buffer.clear();
                this->ClearChars(this->size.column - this->col - 1);
                if (this->line + 1 >= this->size.line || this->line + this->offset.line >= this->term.GetHeight()) {
                    return false;
                } else {
                    this->SetPosition(this->line + 1, 0);
                }
            } else {
                auto curWidth = this->charWidth.GetCharWidth(codepoint);
                if (this->col + curWidth < this->size.column) {
                    buffer.append(view.substr(start, length));
                    this->col += curWidth;
                }
            }
            return true;
        });
        if (!buffer.empty()) {
            this->term.Write(buffer);
        }
    }

    void TerminalWindow::SetGraphicsMode(SlokedTextGraphics mode) {
        this->term.SetGraphicsMode(mode);
    }

    void TerminalWindow::SetGraphicsMode(SlokedBackgroundGraphics mode) {
        this->term.SetGraphicsMode(mode);
    }

    void TerminalWindow::SetGraphicsMode(SlokedForegroundGraphics mode) {
        this->term.SetGraphicsMode(mode);
    }

    void TerminalWindow::Update() {
        this->term.Update();
    }

    void TerminalWindow::Flush(bool f) {
        this->term.Flush(f);
    }
}