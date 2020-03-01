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

#include "sloked/screen/terminal/SDL.h"
#include "sloked/core/Error.h"
#include "sloked/core/Locale.h"

namespace sloked {

    SlokedSDLTerminal::SlokedSDLTerminal(SlokedSDLFont font)
        : font(std::move(font)), size{0, 0}, buffer{nullptr}, cursor{0, 0}, showCursor{true}, backgroundColor{255, 255, 255, 0} {
        if (!this->font.IsFixedWidth()) {
            throw SlokedError("SDLTerminal: Expected monospaced font");
        }
        this->glyphSize = this->font.SizeOf(" ");
    }

    void SlokedSDLTerminal::SetSize(SDL_Point realSize) {
        this->size = {
            static_cast<TextPosition::Line>(realSize.y / this->glyphSize.y),
            static_cast<TextPosition::Column>(realSize.x / this->glyphSize.x)
        };
        this->buffer = SlokedSDLSurface(realSize);
        this->buffer.Fill({0, 0, realSize.x, realSize.y}, this->backgroundColor);
    }

    void SlokedSDLTerminal::PollEvents(SlokedSDLEventQueue &events) {
        while (events.HasEvents()) {
            auto event = events.NextEvent();
        }
    }

    void SlokedSDLTerminal::Render(SlokedSDLSurface &surface) {
        surface.Fill({0, 0, surface.Size().x, surface.Size().y}, {255, 255, 255, 0});
        SDL_BlitSurface(this->buffer.GetSurface(), nullptr, surface.GetSurface(), nullptr);
        if (this->showCursor) {
            surface.Fill({
                this->cursor.column * this->glyphSize.x,
                this->cursor.line * this->glyphSize.y,
                this->glyphSize.x,
                this->glyphSize.y
            }, { 128, 128, 128, 128 });
        }
    }

    void SlokedSDLTerminal::SetPosition(Line l, Column c) {
        this->cursor = {
            std::min(l, this->size.line - 1),
            std::min(c, this->size.column - 1)
        };
    }

    void SlokedSDLTerminal::MoveUp(Line l) {
        this->cursor.line -= std::min(this->cursor.line, l);
    }

    void SlokedSDLTerminal::MoveDown(Line l) {
        this->cursor.line = std::max(this->size.line - 1, this->cursor.line + l);
    }

    void SlokedSDLTerminal::MoveBackward(Column c) {
        this->cursor.column -= std::min(this->cursor.column, c);
    }

    void SlokedSDLTerminal::MoveForward(Column c) {
        this->cursor.column = std::max(this->size.column - 1, this->cursor.column + c);
    }

    void SlokedSDLTerminal::ShowCursor(bool show) {
        this->showCursor = show;
    }

    void SlokedSDLTerminal::ClearScreen() {
        this->buffer.Fill({0, 0, this->buffer.Size().x, this->buffer.Size().y}, this->backgroundColor);
    }

    void SlokedSDLTerminal::ClearChars(Column col) {
        this->buffer.Fill({
            this->cursor.column * this->glyphSize.x,
            this->cursor.line * this->glyphSize.y,
            std::min(col, this->size.column - this->cursor.column) * this->glyphSize.x,
            this->glyphSize.y
        }, this->backgroundColor);
    }

    SlokedSDLTerminal::Column SlokedSDLTerminal::GetWidth() {
        return this->size.column;
    }

    SlokedSDLTerminal::Line SlokedSDLTerminal::GetHeight() {
        return this->size.line;
    }

    static bool ShowLine(std::function<void(std::string_view)> callback, std::string_view content, const Encoding &encoding, TextPosition &cursor, const TextPosition &size) {
        std::size_t start{0}, currentLength{0};
        const auto totalLength = content.size();
        for (Encoding::Iterator it{}; encoding.Iterate(it, content, totalLength);) {
            if (currentLength + 1 == size.column - cursor.column) {
                callback(content.substr(start, currentLength));
                if (cursor.line + 1 == size.line) {
                    return false;
                }
                start = it.start;
                cursor.column = 0;
                cursor.line++;
            }
            currentLength++;
        }
        if (currentLength > 0 || totalLength == 0) {
            callback(content.substr(start, currentLength));
            if (cursor.line + 1 == size.line) {
                return false;
            }
            cursor.column = 0;
            cursor.line++;
        }
        return true;
    }

    static void SplitNewLines(std::vector<std::string_view> &out, std::string_view content, const NewLine &newline) {
        std::size_t start{0};
        newline.Iterate(content, [&](std::size_t nlStart, std::size_t nlLength) {
            out.emplace_back(content.substr(start, nlStart));
            start = nlStart + nlLength;
        });
        if (start < content.size() || content.empty()) {
            out.emplace_back(content.substr(start));
        }
    }

    void SlokedSDLTerminal::Write(std::string_view content) {
        const Encoding &encoding = SlokedLocale::SystemEncoding();
        auto newline = NewLine::LF(encoding);
        std::vector<std::string_view> lines;
        SplitNewLines(lines, content, *newline);
        for (const auto &line : lines) {
            if (!ShowLine([&](auto lineContent) {
                if (lineContent.empty()) {
                    return;
                }
                SlokedSDLSurface lineSurface = this->font.RenderBlended(lineContent, {0, 0, 0, 0});
                SDL_Rect dstRect {
                    this->cursor.column * this->glyphSize.x,
                    this->cursor.line * this->glyphSize.y,
                    (this->size.column - this->cursor.column) * this->glyphSize.x,
                    this->glyphSize.y
                };
                SDL_Rect srcRect {
                    0,
                    0,
                    std::min(static_cast<int>((this->size.column - this->cursor.column) * this->glyphSize.x), lineSurface.Size().x),
                    this->glyphSize.y
                };
                this->buffer.Fill(dstRect, this->backgroundColor);
                SDL_BlitSurface(lineSurface.GetSurface(), &srcRect, this->buffer.GetSurface(), &dstRect);
            }, line, encoding, this->cursor, this->size)) {
                break;
            }
        }
        
        // std::function<bool(std::size_t, std::size_t)> render = [this, content](std::size_t start, std::size_t length) {
        
        //     this->cursor.column = 0;
        //     this->cursor.line++;
        //     if (this->cursor.line >= this->size.line) {
        //         this->cursor.line--;
        //         return false;
        //     }
        //     return true;
        // };
        // bool stop = false;
        // std::size_t lastStart{0};
        // newline->Iterate(content, [&](auto nlStart, auto nlLength) {
        //     if (stop) {
        //         return;
        //     }
        //     stop = !render(lastStart, nlStart);
        //     lastStart = nlStart + nlLength;
        // });
        // if (!stop && lastStart < content.size()) {
        //     render(lastStart, content.size() - lastStart);
        // }
    }

    void SlokedSDLTerminal::SetGraphicsMode(SlokedTextGraphics g) {
        switch (g) {
            case SlokedTextGraphics::Off:
                this->backgroundColor = {255, 255, 255, 0};
                break;
            
            default:
                break;
        }
    }

    void SlokedSDLTerminal::SetGraphicsMode(SlokedBackgroundGraphics bg) {
        switch (bg) {
            case SlokedBackgroundGraphics::Black:
                // this->backgroundColor = {0, 0, 0, 0};
                break;
            // Red,
            // Green,
            // Yellow,
            // Blue,
            // Magenta,
            // Cyan,
            case SlokedBackgroundGraphics::White:
                this->backgroundColor = {255, 255, 255, 0};
                break;

            default:
                this->backgroundColor = {255, 0, 0, 0};
                break;
        }
    }

    void SlokedSDLTerminal::SetGraphicsMode(SlokedForegroundGraphics) {}
}