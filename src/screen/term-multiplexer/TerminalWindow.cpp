#include "sloked/screen/term-multiplexer/TerminalWindow.h"
#include "sloked/core/Encoding.h"
#include <iostream>

namespace sloked {

    TerminalWindow::TerminalWindow(SlokedTerminal &term, const Encoding &encoding, Column x, Line y, Column w, Line h, InputSource inputSource)
        : term(term), encoding(encoding), offset_x(x), offset_y(y), width(w), height(h), inputSource(std::move(inputSource)), col(0), line(0) {}

    void TerminalWindow::Move(Column x, Line y) {
        this->offset_x = x;
        this->offset_y = y;
    }

    void TerminalWindow::Resize(Column w, Line h) {
        this->width = w;
        this->height = h;
    }

    TerminalWindow::Column TerminalWindow::GetOffsetX() const {
        return this->offset_x;
    }

    TerminalWindow::Line TerminalWindow::GetOffsetY() const {
        return this->offset_y;
    }

    TerminalWindow TerminalWindow::SubWindow(Column x, Line y, Column w, Line h, InputSource inputSource) const {
        return TerminalWindow(this->term, this->encoding,
            this->offset_x + x,
            this->offset_y + y,
            w, h, inputSource);
    }

    void TerminalWindow::SetPosition(Line y, Column x) {
        this->line = std::min(y, this->height - 1);
        this->col = std::min(x, this->width - 1);
        this->term.SetPosition(this->offset_y + this->line, this->offset_x + this->col);
    }

    void TerminalWindow::MoveUp(Line l) {
        l = std::min(this->line, l);
        this->line -= l;
        this->term.MoveUp(l);
    }

    void TerminalWindow::MoveDown(Line l) {
        l = std::min(this->line + l, this->height - 1) - this->line;
        this->line += l;
        this->term.MoveDown(l);
    }

    void TerminalWindow::MoveBackward(Column c) {
        c = std::min(this->col, c);
        this->col -= c;
        this->term.MoveBackward(c);
    }

    void TerminalWindow::MoveForward(Column c) {
        c = std::min(this->col + c, this->width - 1) - this->col;
        this->col += c;
        this->term.MoveForward(c);
    }

    void TerminalWindow::ShowCursor(bool show) {
        this->term.ShowCursor(show);
    }

    void TerminalWindow::ClearScreen() {
        std::string cl(this->width, ' ');
        for (std::size_t y = this->offset_y; y < this->offset_y + this->height; y++) {
            this->term.SetPosition(y, this->offset_x);
            this->term.Write(cl);
        }
    }

    void TerminalWindow::ClearChars(Column c) {
        c = std::min(this->col + c, this->width - 1) - this->col;
        this->term.ClearChars(c);
    }

    TerminalWindow::Column TerminalWindow::GetWidth() {
        return this->width;
    }

    TerminalWindow::Line TerminalWindow::GetHeight() {
        return this->height;
    }

    void TerminalWindow::Write(const std::string &str) {
        std::string buffer;
        std::string_view view = str;
        std::size_t count = 0;
        bool res = this->encoding.IterateCodepoints(str, [&](auto start, auto length, auto codepoint) {
            if (codepoint == U'\n') {
                this->term.Write(buffer);
                buffer.clear();
                this->ClearChars(this->width - (this->col + count) - 1);
                count = 0;
                if (this->line + 1 == this->height) {
                    return false;
                }
                this->SetPosition(this->line + 1, 0);
            } else {
                if (this->col + count + 1 < this->width) {;
                    buffer.append(view.substr(start, length));
                    count++;
                }
            }
            return true;
        });
        if (res && !buffer.empty()) {
            this->term.Write(buffer);
        }
    }

    std::vector<SlokedKeyboardInput> TerminalWindow::GetInput() {
        return this->inputSource();
    }

    void TerminalWindow::SetGraphicsMode(SlokedTerminalText mode) {
        this->term.SetGraphicsMode(mode);
    }

    void TerminalWindow::SetGraphicsMode(SlokedTerminalBackground mode) {
        this->term.SetGraphicsMode(mode);
    }

    void TerminalWindow::SetGraphicsMode(SlokedTerminalForeground mode) {
        this->term.SetGraphicsMode(mode);
    }

    void TerminalWindow::Flush(bool f) {
        this->term.Flush(f);
    }
}