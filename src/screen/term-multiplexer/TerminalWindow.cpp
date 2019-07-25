#include "sloked/screen/term-multiplexer/TerminalWindow.h"
#include <iostream>

namespace sloked {

    TerminalWindow::TerminalWindow(SlokedTerminal &term, Column x, Line y, Column w, Line h, InputSource inputSource)
        : term(term), offset_x(x), offset_y(y), width(w), height(h), inputSource(std::move(inputSource)), col(0), line(0) {}

    TerminalWindow::Column TerminalWindow::GetOffsetX() const {
        return this->offset_x;
    }

    TerminalWindow::Line TerminalWindow::GetOffsetY() const {
        return this->offset_y;
    }

    TerminalWindow TerminalWindow::SubWindow(Column x, Line y, Column w, Line h, InputSource inputSource) const {
        return TerminalWindow(this->term,
            this->offset_x + x,
            this->offset_y + y,
            w, h, inputSource);
    }

    void TerminalWindow::SetPosition(Line y, Column x) {
        if (y < this->height && x < this->width) {
            this->line = y;
            this->col = x;
            this->term.SetPosition(this->offset_y + y, this->offset_x + x);
        }
    }

    void TerminalWindow::MoveUp(Line l) {
        if (this->line >= l) {
            this->line -= l;
            this->term.MoveUp(l);
        }
    }

    void TerminalWindow::MoveDown(Line l) {
        if (this->line + l < this->height) {
            this->line += l;
            this->term.MoveDown(l);
        }
    }

    void TerminalWindow::MoveBackward(Column c) {
        if (this->col >= c) {
            this->col -= c;
            this->term.MoveBackward(c);
        }
    }

    void TerminalWindow::MoveForward(Column c) {
        if (this->col + c < this->width) {
            this->col += c;
            this->term.MoveForward(c);
        }
    }

    void TerminalWindow::ShowCursor(bool show) {
        this->term.ShowCursor(show);
    }

    void TerminalWindow::ClearScreen() {
        std::string cl(this->width, ' ');
        for (std::size_t y = this->offset_y; y <= this->offset_y + this->height; y++) {
            this->term.SetPosition(y, this->offset_x);
            this->term.Write(cl);
        }
    }

    void TerminalWindow::ClearChars(Column c) {
        if (this->col + c < this->width) {
            this->term.ClearChars(c);
        }
    }

    TerminalWindow::Column TerminalWindow::GetWidth() {
        return this->width;
    }

    TerminalWindow::Line TerminalWindow::GetHeight() {
        return this->height;
    }

    void TerminalWindow::Write(const std::string &str) {
        std::string buffer;
        for (const auto chr : str) {
            if (chr == '\n') {
                this->term.Write(buffer);
                this->ClearChars(this->width - (this->col + buffer.size()) - 1);
                buffer.clear();
                this->SetPosition(this->line + 1, 0);
            } else {
                if (buffer.size() == this->width) {
                    this->term.Write(buffer);
                    buffer.clear();
                    this->SetPosition(this->line + 1, 0);
                }
                buffer.push_back(chr);
            }
        }
        if (!buffer.empty()) {
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
}