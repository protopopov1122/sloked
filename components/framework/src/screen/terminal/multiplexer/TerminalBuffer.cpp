/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#include "sloked/screen/terminal/multiplexer/TerminalBuffer.h"
#include "sloked/core/Encoding.h"
#include <iostream>
#include <cstring>

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

    BufferedTerminal::BufferedTerminal(SlokedTerminal &term, const Encoding &encoding, const SlokedCharPreset &charPreset)
        : term(term), encoding(encoding), charPreset(charPreset), cls(false), show_cursor(true), buffer(nullptr), line(0), col(0), width(0), height(0) {
        this->UpdateDimensions();
        this->buffer = std::unique_ptr<Character[]>(new Character[this->width * this->height]);
        this->renderBuffer = std::unique_ptr<char32_t[]>(new char32_t[this->width * this->height]);
    }

    void BufferedTerminal::UpdateSize() {
        const auto prevSize = this->width * this->height;
        this->width = term.GetWidth();
        this->height = term.GetHeight();
        const auto newSize = this->width * this->height;
        if (prevSize < newSize) {
            this->buffer = std::unique_ptr<Character[]>(new Character[newSize]);
            this->renderBuffer = std::unique_ptr<char32_t[]>(new char32_t[newSize]);
        }
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
        auto gfx = this->graphics;
        auto area = this->width * this->height;
        for (std::size_t i = 0; i < area; i++) {
            this->buffer[i].value = U' ';
            this->buffer[i].updated = false;
            this->buffer[i].graphics = gfx;
        }
    }

    void BufferedTerminal::ClearChars(Column count) {
        Column max = std::min(this->col + count, this->width) - this->col;
        std::size_t idx = this->line * this->width + this->col;
        auto gfx = this->graphics;
        auto buffer = this->buffer.get();
        while (max--) {
            Character &chr = buffer[idx++];
            chr.value = ' ';
            chr.updated = true;
            chr.graphics = gfx;
        }
    }

    TextPosition::Column BufferedTerminal::GetWidth() {
        return this->width;
    }

    TextPosition::Line BufferedTerminal::GetHeight() {
        return this->height;
    }

    void BufferedTerminal::Write(std::string_view str) {
        const auto length = str.size();
        for (Encoding::Iterator it{}; (it = this->encoding.Iterate(it, str, length)).value != U'\0';) {
            if (it.value == U'\n') {
                this->ClearChars(this->width - this->col);
                if (this->line + 1 == this->height) {
                    break;
                }
                this->SetPosition(this->line + 1, 0);
            } else if (it.value == U'\t') {
                this->Write(this->charPreset.GetTab(this->encoding));
            } else if (this->col < this->width) {
                Character &chr = this->buffer[this->line * this->width + this->col];
                chr.value = it.value;
                chr.updated = true;
                chr.graphics = this->graphics;
                this->col++;
            }
        }
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

    void BufferedTerminal::UpdateDimensions() {
        this->term.UpdateDimensions();
        this->UpdateSize();
    }

    void BufferedTerminal::RenderFrame() {
        this->term.Flush(false);
        this->term.ShowCursor(false);
        const auto fullSize = this->width * this->height;
        char32_t *buffer = this->renderBuffer.get();
        std::size_t buffer_ptr = 0;
        std::size_t buffer_start = 0;

        BufferedGraphicsMode prev_g;
        for (std::size_t i = 0; i < fullSize; i++) {
            Character &chr = this->buffer[i];
            if (chr.graphics.has_value() && !(chr.graphics.value() == prev_g)) {
                this->dump_buffer(std::u32string_view(buffer, buffer_ptr), buffer_start);
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
                this->dump_buffer(std::u32string_view(buffer, buffer_ptr), buffer_start);
                buffer_ptr = 0;
            }
            chr.updated = false;
            chr.graphics.reset();
        }
        this->cls = false;
        this->dump_buffer(std::u32string_view(buffer, buffer_ptr), buffer_start);
        this->term.SetPosition(this->line, this->col);
        this->term.ShowCursor(this->show_cursor);
        this->term.Flush(true);
    }
}