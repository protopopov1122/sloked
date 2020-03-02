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
            this->text = 0;
            this->background = None;
            this->foreground = None;
        }
        this->text |= 1 << static_cast<int>(mode);
    }

    void BufferedGraphicsMode::SetGraphicsMode(SlokedBackgroundGraphics mode) {
        this->background = static_cast<uint32_t>(mode);
    }

    void BufferedGraphicsMode::SetGraphicsMode(SlokedForegroundGraphics mode) {
        this->foreground = static_cast<uint32_t>(mode);
    }

    template <typename T, std::size_t Max, std::size_t Index = 0>
    void ApplyTextMode(SlokedTerminal &term, T value) {
        if constexpr (Index < Max) {
            constexpr T mask = static_cast<T>(1) << Index;
            if (value & mask) {
                term.SetGraphicsMode(static_cast<SlokedTextGraphics>(Index));
            }
            ApplyTextMode<T, Max, Index + 1>(term, value);
        }
    }

    void BufferedGraphicsMode::apply(SlokedTerminal &term) const {
        ApplyTextMode<decltype(this->text), TextSize>(term, this->text);
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

    bool BufferedGraphicsMode::operator!=(const BufferedGraphicsMode &g) const {
        return this->text != g.text ||
            this->background != g.background ||
            this->foreground != g.foreground;
    }

    BufferedTerminal::BufferedTerminal(SlokedTerminal &term, const Encoding &encoding, const SlokedCharPreset &charPreset)
        : term(term), encoding(encoding), charPreset(charPreset), show_cursor(true), current_state(nullptr), prev_state(nullptr), line(0), col(0), width(0), height(0) {
        this->UpdateDimensions();
        const auto length = this->width * this->height;
        this->current_state = std::unique_ptr<Character[]>(new Character[length]);
        this->prev_state = std::unique_ptr<Character[]>(new Character[length]);
        this->renderBuffer = std::unique_ptr<char32_t[]>(new char32_t[length]);
    }

    void BufferedTerminal::UpdateSize() {
        const auto prevSize = this->width * this->height;
        this->width = term.GetWidth();
        this->height = term.GetHeight();
        const auto newSize = this->width * this->height;
        if (prevSize < newSize) {
            this->current_state = std::unique_ptr<Character[]>(new Character[newSize]);
            this->prev_state = std::unique_ptr<Character[]>(new Character[newSize]);
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
        auto gfx = this->graphics;
        auto buffer = this->current_state.get();
        uint_fast32_t area = this->width * this->height;
        for (uint_fast32_t i = 0; i < area; i++) {
            Character &chr = *buffer++;
            chr.has_graphics = true;
            chr.graphics = gfx;
            chr.value = U' ';
        }
    }

    void BufferedTerminal::ClearChars(Column count) {
        Column max = std::min(this->col + count, this->width) - this->col;
        std::size_t idx = this->line * this->width + this->col;
        auto gfx = this->graphics;
        auto buffer = this->current_state.get();
        while (max--) {
            Character &chr = buffer[idx++];
            chr.has_graphics = true;
            chr.graphics = gfx;
            chr.value = ' ';
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
        for (Encoding::Iterator it{}; this->encoding.Iterate(it, str, length);) {
            if (it.value == U'\n') {
                this->ClearChars(this->width - this->col);
                if (this->line + 1 == this->height) {
                    break;
                }
                this->SetPosition(this->line + 1, 0);
            } else if (it.value == U'\t') {
                this->Write(this->charPreset.GetTab(this->encoding));
            } else if (this->col < this->width) {
                Character &chr = this->current_state[this->line * this->width + this->col];
                chr.has_graphics = true;
                chr.graphics = this->graphics;
                chr.value = it.value;
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
        char32_t *render_base = this->renderBuffer.get();
        char32_t *render_end = render_base;
        std::size_t offset = 0;
        auto prev_state = this->prev_state.get();
        auto current_state = this->current_state.get();

        BufferedGraphicsMode prev_g;
        bool prev_has_g{false};
        for (std::size_t i = 0; i < fullSize; i++) {
            Character &chr = current_state[i];
            Character &prevChr = prev_state[i];
            if (chr.has_graphics != prev_has_g || (chr.has_graphics && chr.graphics != prev_g)) {
                if (render_end != render_base) {
                    this->dump_buffer(std::u32string_view(render_base, static_cast<ptrdiff_t>(render_end - render_base)), offset);
                    render_end = render_base;
                }
                chr.graphics.apply(term);
                prev_g = chr.graphics;
                prev_has_g = chr.has_graphics;
            }
            if (chr != prevChr) {
                if (render_base == render_end) {
                    offset = i;
                }
                *(render_end++) = chr.value;
            } else if (render_end != render_base) {
                this->dump_buffer(std::u32string_view(render_base, static_cast<ptrdiff_t>(render_end - render_base)), offset);
                render_end = render_base;
            }
            prevChr = chr;
        }
        this->dump_buffer(std::u32string_view(render_base, static_cast<ptrdiff_t>(render_end - render_base)), offset);
        this->term.SetPosition(this->line, this->col);
        this->term.ShowCursor(this->show_cursor);
        this->term.Flush(true);
    }
}