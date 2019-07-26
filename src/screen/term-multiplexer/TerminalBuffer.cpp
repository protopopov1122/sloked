#include "sloked/screen/term-multiplexer/TerminalBuffer.h"
#include "sloked/core/Encoding.h"
#include <iostream>

namespace sloked {

    void BufferedGraphicsMode::SetGraphicsMode(SlokedTerminalText mode) {
        if (mode == SlokedTerminalText::Off) {
            this->text.clear();
            this->background.reset();
            this->foreground.reset();
        }
        this->text.insert(mode);
    }

    void BufferedGraphicsMode::SetGraphicsMode(SlokedTerminalBackground mode) {
        this->background = mode;
    }

    void BufferedGraphicsMode::SetGraphicsMode(SlokedTerminalForeground mode) {
        this->foreground = mode;
    }

    void BufferedGraphicsMode::apply(SlokedTerminal &term) {
        for (const auto mode : this->text) {
            term.SetGraphicsMode(mode);
        }
        if (this->background.has_value()) {
            term.SetGraphicsMode(this->background.value());
        }
        if (this->foreground.has_value()) {
            term.SetGraphicsMode(this->foreground.value());
        }
    }

    bool BufferedGraphicsMode::operator==(const BufferedGraphicsMode &g) {
        return this->text == g.text &&
            this->background == g.background &&
            this->foreground == g.foreground;
    }

    BufferedTerminal::BufferedTerminal(SlokedTerminal &term, const Encoding &encoding)
        : term(term), encoding(encoding), cls(false), show_cursor(true), buffer(nullptr), graphics(nullptr), line(0), col(0) {
        this->width = term.GetWidth();
        this->height = term.GetHeight();
        this->buffer = new Character[this->width * this->height];
    }

    BufferedTerminal::~BufferedTerminal() {
        delete[] this->buffer;
    }

    void BufferedTerminal::Flush() {
        bool forceUpdate = false;
        if (this->cls) {
            this->term.ClearScreen();
            forceUpdate = true;
        }

        this->term.ShowCursor(false);
        std::string buffer;
        std::size_t buffer_start = 0;
        auto dump_buffer = [&]() {
            if (!buffer.empty()) {
                Line l = buffer_start / this->width;
                Column c = buffer_start % this->width;
                term.SetPosition(l, c);
                term.Write(buffer);
                buffer.clear();
            }
        };
        BufferedGraphicsMode *prev_g = nullptr;
        for (std::size_t i = 0; i < this->width * this->height; i++) {
            if (i % this->width == 0) {
                dump_buffer();
            }
            const Character &chr = this->buffer[i];
            if (chr.graphics != nullptr && (prev_g == nullptr || !(*chr.graphics == *prev_g))) {
                dump_buffer();
                chr.graphics->apply(term);
                prev_g = chr.graphics.get();
            }
            if (chr.value != U'\0' && (chr.updated || forceUpdate)) {
                if (buffer.empty()) {
                    buffer_start = i;
                }
                buffer.append(this->encoding.Encode(chr.value));
            } else {
                dump_buffer();
            }
        }
        dump_buffer();
        this->term.SetPosition(this->line, this->col);
        this->term.ShowCursor(this->show_cursor);
        this->reset();
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
            this->buffer[i] = Character{};
            if (this->graphics) {
                this->buffer[i].graphics = std::make_unique<BufferedGraphicsMode>(*this->graphics);
            }
        }
    }

    void BufferedTerminal::ClearChars(Column count) {
        for (Column col = 0; col < count && this->col + col < this->width; col++) {
            Character &chr = this->buffer[this->line * this->width + this->col + col];
            if (chr.value != U' ') {
                chr.value = ' ';
                chr.updated = true;
            }
        }
    }

    BufferedTerminal::Column BufferedTerminal::GetWidth() {
        return this->width;
    }

    BufferedTerminal::Line BufferedTerminal::GetHeight() {
        return this->height;
    }

    void BufferedTerminal::Write(const std::string &str) {
        Column line_width = 0;
        this->encoding.IterateCodepoints(str, [&](auto start, auto length, auto codepoint) {
            if (codepoint == U'\n') {
                this->ClearChars(this->width - line_width - 1);
                line_width = 0;
                if (this->line + 1 == this->height) {
                    return false;
                }
                this->SetPosition(this->line + 1, 0);
            } else {
                if (this->col + line_width < this->width - 1) {
                    this->buffer[this->line * this->width + this->col].value = codepoint;
                    this->buffer[this->line * this->width + this->col].updated = true;
                    if (this->graphics) {
                        this->buffer[this->line * this->width + this->col].graphics = std::make_unique<BufferedGraphicsMode>(*this->graphics);
                    }
                    this->col++;
                    line_width++;
                }        
            }
            return true;
        });
    }

    std::vector<SlokedKeyboardInput> BufferedTerminal::GetInput() {
        return this->term.GetInput();
    }

    void BufferedTerminal::SetGraphicsMode(SlokedTerminalText mode) {
        if (this->graphics == nullptr) {
            this->graphics = std::make_unique<BufferedGraphicsMode>();
        }
        this->graphics->SetGraphicsMode(mode);
    }

    void BufferedTerminal::SetGraphicsMode(SlokedTerminalBackground mode) {
        if (this->graphics == nullptr) {
            this->graphics = std::make_unique<BufferedGraphicsMode>();
        }
        this->graphics->SetGraphicsMode(mode);
    }

    void BufferedTerminal::SetGraphicsMode(SlokedTerminalForeground mode) {
        if (this->graphics == nullptr) {
            this->graphics = std::make_unique<BufferedGraphicsMode>();
        }
        this->graphics->SetGraphicsMode(mode);
    }

    void BufferedTerminal::reset() {
        this->cls = false;
        for (std::size_t i = 0; i < this->width * this->height; i++) {
            this->buffer[i].updated = false;
            this->buffer[i].graphics = nullptr;
        }
    }
}