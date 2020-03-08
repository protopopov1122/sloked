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

#include "sloked/screen/terminal/CairoTerminal.h"
#include "sloked/core/Encoding.h"
#include "sloked/core/NewLine.h"
#include "sloked/core/Locale.h"

namespace sloked {

    struct SlokedCairoTerminal::Renderer {
        Renderer(Cairo::RefPtr<Cairo::Surface> surface, const std::string &fontDescr)
            : surface(surface),
              context(Cairo::Context::create(surface)),
              fontMap(Glib::wrap(pango_cairo_font_map_get_default())),
              textLayout(Pango::Layout::create(this->context)),
              fontDescription(fontDescr) {
            this->textLayout->set_font_description(this->fontDescription);
            this->font = this->fontMap->load_font(this->textLayout->get_context(), this->fontDescription);
        }

        Cairo::RefPtr<Cairo::Surface> surface;
        Cairo::RefPtr<Cairo::Context> context;
        Glib::RefPtr<Pango::FontMap> fontMap;
        Glib::RefPtr<Pango::Layout> textLayout;
        Pango::FontDescription fontDescription;
        Glib::RefPtr<Pango::Font> font;
    };

    SlokedCairoTerminal::SlokedCairoTerminal(Cairo::RefPtr<Cairo::Surface> surface, Dimensions surfaceSize)
        : renderer(std::make_unique<Renderer>(surface, "Monospace 10")), cursor{0, 0}, surfaceSize{surfaceSize} {
        this->renderer->textLayout->set_text("A");
        this->renderer->textLayout->get_pixel_size(this->glyphSize.x, this->glyphSize.y);
        this->size = {
            surfaceSize.y / this->glyphSize.y,
            surfaceSize.x / this->glyphSize.x
        };
        this->ClearScreen();
    }

    SlokedCairoTerminal::~SlokedCairoTerminal() = default;
    
    void SlokedCairoTerminal::SetPosition(Line l, Column c) {
        this->cursor = {
            std::min(l, this->size.line - 1),
            std::min(c, this->size.column - 1)
        };
    }

    void SlokedCairoTerminal::MoveUp(Line l) {
        this->cursor.line -= std::min(this->cursor.line, l);
    }

    void SlokedCairoTerminal::MoveDown(Line l) {
        this->cursor.line = std::max(this->size.line - 1, this->cursor.line + l);
    }

    void SlokedCairoTerminal::MoveBackward(Column c) {
        this->cursor.column -= std::min(this->cursor.column, c);
    }

    void SlokedCairoTerminal::MoveForward(Column c) {
        this->cursor.column = std::max(this->size.column - 1, this->cursor.column + c);
    }

    void SlokedCairoTerminal::ShowCursor(bool show) {
        this->showCursor = show;
    }

    void SlokedCairoTerminal::ClearScreen() {
        this->renderer->context->set_source_rgba(1, 1, 1, 1.0);
        this->renderer->context->rectangle(0, 0, this->surfaceSize.x, this->surfaceSize.y);
        this->renderer->context->fill();
    }

    void SlokedCairoTerminal::ClearChars(Column col) {
        this->renderer->context->set_source_rgba(1, 1, 1, 1.0);
        this->renderer->context->rectangle(this->cursor.column * this->glyphSize.x,
            this->cursor.line * this->glyphSize.y,
            std::min(col, this->size.column - this->cursor.column) * this->glyphSize.x,
            this->glyphSize.y);
        this->renderer->context->fill();
    }

    SlokedCairoTerminal::Column SlokedCairoTerminal::GetWidth() {
        return this->size.column;
    }

    SlokedCairoTerminal::Line SlokedCairoTerminal::GetHeight() {
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

    void SlokedCairoTerminal::Write(std::string_view content) {
        const Encoding &encoding = SlokedLocale::SystemEncoding();
        auto newline = NewLine::LF(encoding);
        std::vector<std::string_view> lines;
        SplitNewLines(lines, content, *newline);
        for (const auto &line : lines) {
            if (!ShowLine([&](auto lineContent) {
                if (lineContent.empty()) {
                    return;
                }
                this->renderer->textLayout->set_text({lineContent.begin(), lineContent.end()});
                Dimensions sz{0, 0};
                this->renderer->textLayout->get_pixel_size(sz.x, sz.y);
                this->renderer->context->set_source_rgba(1, 1, 1, 1.0);
                this->renderer->context->rectangle(this->cursor.column * this->glyphSize.x, this->cursor.line * this->glyphSize.y, sz.x, sz.y);
                this->renderer->context->fill();
                this->renderer->context->move_to(this->cursor.column * this->glyphSize.x, this->cursor.line * this->glyphSize.y);
                this->renderer->context->set_source_rgba(0, 0, 0, 1.0);
                this->renderer->textLayout->show_in_cairo_context(this->renderer->context);
            }, line, encoding, this->cursor, this->size)) {
                break;
            }
        }
    }
        


    void SlokedCairoTerminal::SetGraphicsMode(SlokedTextGraphics) {}

    void SlokedCairoTerminal::SetGraphicsMode(SlokedBackgroundGraphics) {}

    void SlokedCairoTerminal::SetGraphicsMode(SlokedForegroundGraphics) {}
}