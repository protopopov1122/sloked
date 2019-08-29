/*
  SPDX-License-Identifier: LGPL-3.0-or-later

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/screen/terminal/multiplexer/TerminalBuffer.h"
#include "sloked/core/Encoding.h"
#include <iostream>

namespace sloked {

    void BufferedGraphicsMode::SetGraphicsMode(SlokedTextGraphics mode) {
        if (mode == SlokedTextGraphics::Off) {
            this->text.reset();
            this->background = None;
            this->foreground = None;
        }
        this->text.set(static_cast<int>(mode));
    }

    void BufferedGraphicsMode::SetGraphicsMode(SlokedBackgroundGraphics mode) {
        this->background = static_cast<uint32_t>(mode);
    }

    void BufferedGraphicsMode::SetGraphicsMode(SlokedForegroundGraphics mode) {
        this->foreground = static_cast<uint32_t>(mode);
    }

    void BufferedGraphicsMode::apply(SlokedTerminal &term) const {
        for (std::size_t i = 0; i < this->text.size(); i++) {
            if (text[i]) {
                term.SetGraphicsMode(static_cast<SlokedTextGraphics>(i));
            }
        }
        if (this->background != None) {
            term.SetGraphicsMode(static_cast<SlokedBackgroundGraphics>(this->background));
        }
        if (this->foreground != None) {
            term.SetGraphicsMode(static_cast<SlokedForegroundGraphics>(this->foreground));
        }
    }

    bool BufferedGraphicsMode::operator==(const BufferedGraphicsMode &g) const {
        return this->text == g.text &&
            this->background == g.background &&
            this->foreground == g.foreground;
    }

    BufferedTerminal::BufferedTerminal(SlokedTerminal &term, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : term(term), encoding(encoding), charWidth(charWidth), cls(false), show_cursor(true), buffer(nullptr), line(0), col(0) {
        this->Update();
        this->buffer = new Character[this->width * this->height];
    }

    BufferedTerminal::~BufferedTerminal() {
        delete[] this->buffer;
    }

    void BufferedTerminal::Flush() {
        this->term.Flush(false);
        this->term.ShowCursor(false);
        std::unique_ptr<char32_t[]> buffer(new char32_t[this->width * this->height]);
        std::size_t buffer_ptr = 0;
        std::size_t buffer_start = 0;

        BufferedGraphicsMode prev_g;
        for (std::size_t i = 0; i < this->width * this->height; i++) {
            Character &chr = this->buffer[i];
            if (chr.graphics.has_value() && !(chr.graphics.value() == prev_g)) {
                this->dump_buffer(std::u32string_view(buffer.get(), buffer_ptr), buffer_start);
                buffer_ptr = 0;
                chr.graphics.value().apply(term);
                prev_g = chr.graphics.value();
            }
            if (chr.value != U'\0' && (chr.updated || this->cls)) {
                if (!buffer_ptr) {
                    buffer_start = i;
                }
                buffer[buffer_ptr++] = chr.value;
            } else {
                this->dump_buffer(std::u32string_view(buffer.get(), buffer_ptr), buffer_start);
                buffer_ptr = 0;
            }
            chr.updated = false;
            chr.graphics.reset();
        }
        this->cls = false;
        this->dump_buffer(std::u32string_view(buffer.get(), buffer_ptr), buffer_start);
        this->term.SetPosition(this->line, this->col);
        this->term.ShowCursor(this->show_cursor);
        this->term.Flush(true);
    }

    void BufferedTerminal::UpdateSize() {
        this->width = term.GetWidth();
        this->height = term.GetHeight();
        delete[] this->buffer;
        this->buffer = new Character[this->width * this->height];
    }
    
    void BufferedTerminal::SetPosition(Line l, Column c) {
        if (l < this->height && c < this->width) {
            this->line = l;
            this->col = c;
        }
    }

    void BufferedTerminal::MoveUp(Line l) {
        if (this->line >= l) {
            this->line -= l;
        }
    }

    void BufferedTerminal::MoveDown(Line l) {
        if (this->line + l < this->height) {
            this->line += l;
        }
    }

    void BufferedTerminal::MoveBackward(Column c) {
        if (this->col >= c) {
            this->col -=c;
        }
    }

    void BufferedTerminal::MoveForward(Column c) {
        if (this->col + c < this->width) {
            this->col += c;
        }
    }

    void BufferedTerminal::ShowCursor(bool show) {
        this->show_cursor = show;
    }

    void BufferedTerminal::ClearScreen() {
        this->cls = true;
        for (std::size_t i = 0; i < this->width * this->height; i++) {
            this->buffer[i].value = U' ';
            this->buffer[i].updated = false;
            this->buffer[i].graphics = this->graphics;
        }
    }

    void BufferedTerminal::ClearChars(Column count) {
        Column max = std::min(this->col + count, this->width) - this->col;
        std::size_t idx = this->line * this->width + this->col;
        while (max--) {
            Character &chr = this->buffer[idx++];
            if (chr.value != U' ') {
                chr.value = ' ';
                chr.updated = true;
            }
        }
    }

    TextPosition::Column BufferedTerminal::GetWidth() {
        return this->width;
    }

    TextPosition::Line BufferedTerminal::GetHeight() {
        return this->height;
    }

    void BufferedTerminal::Write(const std::string &str) {
        this->encoding.IterateCodepoints(str, [&](auto start, auto length, auto codepoint) {
            if (codepoint == U'\n') {
                this->ClearChars(this->width - this->col);
                if (this->line + 1 == this->height) {
                    return false;
                }
                this->SetPosition(this->line + 1, 0);
            } else if (codepoint == U'\t') {
                this->Write(this->charWidth.GetTab());
            } else if (this->col < this->width) {
                Character &chr = this->buffer[this->line * this->width + this->col];
                chr.value = codepoint;
                chr.updated = true;
                chr.graphics = this->graphics;
                this->col++;
            }
            return true;
        });
    }

    void BufferedTerminal::SetGraphicsMode(SlokedTextGraphics mode) {
        this->graphics.SetGraphicsMode(mode);
    }

    void BufferedTerminal::SetGraphicsMode(SlokedBackgroundGraphics mode) {
        this->graphics.SetGraphicsMode(mode);
    }

    void BufferedTerminal::SetGraphicsMode(SlokedForegroundGraphics mode) {
        this->graphics.SetGraphicsMode(mode);
    }

    void BufferedTerminal::dump_buffer(std::u32string_view str, std::size_t buffer_start) {
        if (!str.empty()) {
            Line l = buffer_start / this->width;
            Column c = buffer_start % this->width;
            this->term.SetPosition(l, c);
            this->term.Write(this->encoding.Encode(str));
        }
    }

    void BufferedTerminal::Update() {
        this->term.Update();
        this->UpdateSize();
    }
}