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

    namespace CairoColors {
        static auto Black = Cairo::SolidPattern::create_rgb(0, 0, 0);
        static auto Red = Cairo::SolidPattern::create_rgb(1, 0, 0);
        static auto Green = Cairo::SolidPattern::create_rgb(0, 1, 0);
        static auto Yellow = Cairo::SolidPattern::create_rgb(1, 1, 0);
        static auto Blue = Cairo::SolidPattern::create_rgb(0, 0, 1);
        static auto Magenta = Cairo::SolidPattern::create_rgb(1, 0, 1);
        static auto Cyan = Cairo::SolidPattern::create_rgb(0, 1, 1);
        static auto White = Cairo::SolidPattern::create_rgb(1, 1, 1);
    }

    struct SlokedCairoTerminal::Renderer {
        Renderer(Dimensions &dim, const std::string &fontDescr)
            : surface(Cairo::ImageSurface::create(
                Cairo::Format::FORMAT_RGB24,
                dim.x,
                dim.y)),
              context(Cairo::Context::create(surface)),
              fontMap(Glib::wrap(pango_cairo_font_map_get_default())),
              textLayout(Pango::Layout::create(this->context)),
              normalFont(fontDescr),
              boldFont(Glib::wrap(normalFont.gobj_copy())),
              foregroundColor(CairoColors::Black),
              backgroundColor(CairoColors::White) {
            this->boldFont.set_weight(Pango::Weight::WEIGHT_BOLD);
            this->textLayout->set_font_description(this->normalFont);
            this->font = this->fontMap->load_font(this->textLayout->get_context(), this->normalFont);
        }

        Cairo::RefPtr<Cairo::Surface> surface;
        Cairo::RefPtr<Cairo::Context> context;
        Glib::RefPtr<Pango::FontMap> fontMap;
        Glib::RefPtr<Pango::Layout> textLayout;
        Pango::FontDescription normalFont;
        Pango::FontDescription boldFont;
        Glib::RefPtr<Pango::Font> font;
        Cairo::RefPtr<Cairo::SolidPattern> foregroundColor;
        Cairo::RefPtr<Cairo::SolidPattern> backgroundColor;
    };

    SlokedCairoTerminal::SlokedCairoTerminal(Dimensions surfaceSize)
        : renderer(std::make_unique<Renderer>(surfaceSize, "Monospace 10")), cursor{0, 0}, surfaceSize{surfaceSize} {
        this->renderer->textLayout->set_text("A");
        this->renderer->textLayout->get_pixel_size(this->glyphSize.x, this->glyphSize.y);
        this->size = {
            surfaceSize.y / this->glyphSize.y,
            surfaceSize.x / this->glyphSize.x
        };
        this->ClearScreen();
    }

    SlokedCairoTerminal::~SlokedCairoTerminal() = default;

    void SlokedCairoTerminal::Render(Cairo::RefPtr<Cairo::Context> targetCtx) {
        targetCtx->set_source(this->renderer->surface, 0.0, 0.0);
        targetCtx->paint();
        if (this->showCursor) {
            struct {
                double red;
                double green;
                double blue;
                double alpha;
            } cursorColor;
            this->renderer->backgroundColor->get_rgba(cursorColor.red, cursorColor.green, cursorColor.blue, cursorColor.alpha);
            cursorColor.red = 1.0 - cursorColor.red;
            cursorColor.green = 1.0 - cursorColor.green;
            cursorColor.blue = 1.0 - cursorColor.blue;
            targetCtx->set_source_rgba(cursorColor.red, cursorColor.green, cursorColor.blue, 0.3);
            targetCtx->rectangle(this->cursor.column * this->glyphSize.x, this->cursor.line * this->glyphSize.y, this->glyphSize.x, this->glyphSize.y);
            targetCtx->fill();
        }
    }
    
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
        this->renderer->context->set_source(this->renderer->backgroundColor);
        this->renderer->context->rectangle(0, 0, this->surfaceSize.x, this->surfaceSize.y);
        this->renderer->context->fill();
    }

    void SlokedCairoTerminal::ClearChars(Column col) {
        this->renderer->context->set_source(this->renderer->backgroundColor);
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
                this->renderer->context->set_source(this->renderer->backgroundColor);
                this->renderer->context->rectangle(this->cursor.column * this->glyphSize.x, this->cursor.line * this->glyphSize.y, sz.x, sz.y);
                this->renderer->context->fill();
                this->renderer->context->move_to(this->cursor.column * this->glyphSize.x, this->cursor.line * this->glyphSize.y);
                this->renderer->context->set_source(this->renderer->foregroundColor);
                this->renderer->textLayout->show_in_cairo_context(this->renderer->context);
            }, line, encoding, this->cursor, this->size)) {
                break;
            }
        }
    }

    void SlokedCairoTerminal::SetGraphicsMode(SlokedTextGraphics mode) {
        switch (mode) {
            case SlokedTextGraphics::Off:
                this->renderer->foregroundColor = CairoColors::Black;
                this->renderer->backgroundColor = CairoColors::White;
                this->renderer->textLayout->set_font_description(this->renderer->normalFont);
                break;

            case SlokedTextGraphics::Bold:
                this->renderer->textLayout->set_font_description(this->renderer->boldFont);
                break;

            case SlokedTextGraphics::Reverse:
                std::swap(this->renderer->foregroundColor, this->renderer->backgroundColor);
                break;

            default:
                // Not supported yet
                break;
        }
    }

    void SlokedCairoTerminal::SetGraphicsMode(SlokedBackgroundGraphics color) {
        switch (color) {
            case SlokedBackgroundGraphics::Black:
                this->renderer->backgroundColor = CairoColors::Black;
                break;

            case SlokedBackgroundGraphics::Red:
                this->renderer->backgroundColor = CairoColors::Red;
                break;

            case SlokedBackgroundGraphics::Green:
                this->renderer->backgroundColor = CairoColors::Green;
                break;

            case SlokedBackgroundGraphics::Yellow:
                this->renderer->backgroundColor = CairoColors::Black;
                break;

            case SlokedBackgroundGraphics::Blue:
                this->renderer->backgroundColor = CairoColors::Blue;
                break;
            case SlokedBackgroundGraphics::Magenta:
                this->renderer->backgroundColor = CairoColors::Magenta;
                break;

            case SlokedBackgroundGraphics::Cyan:
                this->renderer->backgroundColor = CairoColors::Cyan;
                break;

            case SlokedBackgroundGraphics::White:
                this->renderer->backgroundColor = CairoColors::White;
                break;
        }
    }

    void SlokedCairoTerminal::SetGraphicsMode(SlokedForegroundGraphics color) {
        switch (color) {
            case SlokedForegroundGraphics::Black:
                this->renderer->foregroundColor = CairoColors::Black;
                break;

            case SlokedForegroundGraphics::Red:
                this->renderer->foregroundColor = CairoColors::Red;
                break;

            case SlokedForegroundGraphics::Green:
                this->renderer->foregroundColor = CairoColors::Green;
                break;

            case SlokedForegroundGraphics::Yellow:
                this->renderer->foregroundColor = CairoColors::Black;
                break;

            case SlokedForegroundGraphics::Blue:
                this->renderer->foregroundColor = CairoColors::Blue;
                break;

            case SlokedForegroundGraphics::Magenta:
                this->renderer->foregroundColor = CairoColors::Magenta;
                break;

            case SlokedForegroundGraphics::Cyan:
                this->renderer->foregroundColor = CairoColors::Cyan;
                break;

            case SlokedForegroundGraphics::White:
                this->renderer->foregroundColor = CairoColors::White;
                break;
        }
    }
}