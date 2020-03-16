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
#include "sloked/core/Event.h"

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
        Renderer(const Dimensions &dim, const std::string &fontDescr)
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
        }

        void Resize(const Dimensions &dim) {
            auto newSurface = Cairo::ImageSurface::create(
                Cairo::Format::FORMAT_RGB24,
                dim.x,
                dim.y);
            auto newContext = Cairo::Context::create(newSurface);
            auto newTextLayout = Pango::Layout::create(this->context);
            newTextLayout->set_text(this->textLayout->get_text());
            newTextLayout->set_font_description(this->textLayout->get_font_description());
            auto attrs = this->textLayout->get_attributes();
            newTextLayout->set_attributes(attrs);
            this->textLayout = newTextLayout;
            this->context = newContext;
            this->surface = newSurface;
        }

        Cairo::RefPtr<Cairo::Surface> surface;
        Cairo::RefPtr<Cairo::Context> context;
        Glib::RefPtr<Pango::FontMap> fontMap;
        Glib::RefPtr<Pango::Layout> textLayout;
        Pango::FontDescription normalFont;
        Pango::FontDescription boldFont;
        Cairo::RefPtr<Cairo::SolidPattern> foregroundColor;
        Cairo::RefPtr<Cairo::SolidPattern> backgroundColor;
        std::mutex mtx;
    };

    class SlokedCairoTerminal::Size : public SlokedScreenSize {
     public:
        Size(SlokedCairoTerminal &terminal)
            : terminal(terminal) {}

        TextPosition GetScreenSize() const final {
            return {
                terminal.GetHeight(),
                terminal.GetWidth()
            };
        }
        
        std::function<void()> Listen(Listener listener) final {
            return this->emitter.Listen(std::move(listener));
        }

        void Notify() {
            this->emitter.Emit(this->GetScreenSize());
        }

     private:
        SlokedCairoTerminal &terminal;
        SlokedEventEmitter<TextPosition> emitter;
    };

    SlokedCairoTerminal::SlokedCairoTerminal(Dimensions surfaceSize, const std::string &font)
        : renderer(std::make_unique<Renderer>(surfaceSize, font)), screenSize(std::make_unique<Size>(*this)),
          cursor{0, 0}, surfaceSize{surfaceSize}, updated{true}, input{InputBufferSize} {
        this->renderer->textLayout->set_text("A");
        this->renderer->textLayout->get_pixel_size(this->glyphSize.x, this->glyphSize.y);
        this->size = {
            static_cast<TextPosition::Line>(surfaceSize.y / this->glyphSize.y),
            static_cast<TextPosition::Column>(surfaceSize.x / this->glyphSize.x)
        };
    }

    SlokedCairoTerminal::~SlokedCairoTerminal() = default;

    SlokedScreenSize &SlokedCairoTerminal::GetTerminalSize() {
        return *this->screenSize;
    }

    bool SlokedCairoTerminal::HasUpdates() const {
        return this->updated.load();
    }

    void SlokedCairoTerminal::ProcessInput(std::vector<SlokedKeyboardInput> events) {
        std::unique_lock lock(this->input_mtx);
        if (this->input.available() < events.size()) {
            this->input.pop_front(events.size() - this->input.available());
        }
        this->input.insert(events.begin(), events.end());
        this->input_cv.notify_all();
    }

    void SlokedCairoTerminal::Render(const Cairo::RefPtr<Cairo::Context> &targetCtx) {
        std::unique_lock lock(this->renderer->mtx);
        this->updated = false;
        targetCtx->set_source(this->renderer->surface, 0.0, 0.0);
        targetCtx->paint();
        if (this->showCursor) {
            targetCtx->set_source_rgb(1.0, 1.0, 1.0);
            targetCtx->set_operator(static_cast<Cairo::Operator>(CAIRO_OPERATOR_DIFFERENCE));
            targetCtx->rectangle(this->cursor.column * this->glyphSize.x, this->cursor.line * this->glyphSize.y, this->glyphSize.x, this->glyphSize.y);
            targetCtx->fill();
            targetCtx->set_operator(Cairo::Operator::OPERATOR_SOURCE);
        }
    }

    void SlokedCairoTerminal::SetSize(Dimensions dim) {
        std::unique_lock lock(this->renderer->mtx);
        this->renderer->Resize(dim);
        this->renderer->textLayout->set_text("A");
        this->renderer->textLayout->get_pixel_size(this->glyphSize.x, this->glyphSize.y);
        this->surfaceSize = dim;
        this->size = {
            static_cast<TextPosition::Line>(this->surfaceSize.y / this->glyphSize.y),
            static_cast<TextPosition::Column>(this->surfaceSize.x / this->glyphSize.x)
        };
        this->renderer->foregroundColor = CairoColors::Black;
        this->renderer->backgroundColor = CairoColors::White;
        this->renderer->textLayout->set_font_description(this->renderer->normalFont);
        Pango::AttrList attrs;
        this->renderer->textLayout->set_attributes(attrs);
        this->updated = true;
        this->screenSize->Notify();
    }

    SlokedCairoTerminal::Dimensions SlokedCairoTerminal::GetSize() const {
        return this->surfaceSize;
    }
    
    void SlokedCairoTerminal::SetPosition(Line l, Column c) {
        this->updated = true;
        this->cursor = {
            std::min(l, this->size.line - 1),
            std::min(c, this->size.column - 1)
        };
    }

    void SlokedCairoTerminal::MoveUp(Line l) {
        this->updated = true;
        this->cursor.line -= std::min(this->cursor.line, l);
    }

    void SlokedCairoTerminal::MoveDown(Line l) {
        this->updated = true;
        this->cursor.line = std::max(this->size.line - 1, this->cursor.line + l);
    }

    void SlokedCairoTerminal::MoveBackward(Column c) {
        this->updated = true;
        this->cursor.column -= std::min(this->cursor.column, c);
    }

    void SlokedCairoTerminal::MoveForward(Column c) {
        this->updated = true;
        this->cursor.column = std::max(this->size.column - 1, this->cursor.column + c);
    }

    void SlokedCairoTerminal::ShowCursor(bool show) {
        this->updated = true;
        this->showCursor = show;
    }

    void SlokedCairoTerminal::ClearScreen() {
        this->updated = true;
        std::unique_lock lock(this->renderer->mtx);
        this->renderer->context->set_source(this->renderer->backgroundColor);
        this->renderer->context->rectangle(0, 0, this->surfaceSize.x, this->surfaceSize.y);
        this->renderer->context->fill();
    }

    void SlokedCairoTerminal::ClearChars(Column col) {
        this->updated = true;
        std::unique_lock lock(this->renderer->mtx);
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
            if (it.value == U'\0') {
                break;
            }
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
        this->updated = true;
        std::unique_lock lock(this->renderer->mtx);
        const Encoding &encoding = SlokedLocale::SystemEncoding();
        auto newline = NewLine::LF(encoding);
        std::vector<std::string_view> lines;
        SplitNewLines(lines, content, *newline);
        for (const auto &line : lines) {
            if (!ShowLine([&](auto lineContent) {
                if (lineContent.empty()) {
                    return;
                }
                this->renderer->textLayout->set_text({lineContent.data(), lineContent.size()});
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
        std::unique_lock lock(this->renderer->mtx);
        this->updated = true;
        switch (mode) {
            case SlokedTextGraphics::Off: {
                this->renderer->foregroundColor = CairoColors::Black;
                this->renderer->backgroundColor = CairoColors::White;
                this->renderer->textLayout->set_font_description(this->renderer->normalFont);
                Pango::AttrList attrs;
                this->renderer->textLayout->set_attributes(attrs);
            } break;

            case SlokedTextGraphics::Bold:
                this->renderer->textLayout->set_font_description(this->renderer->boldFont);
                break;

            case SlokedTextGraphics::Reverse:
                std::swap(this->renderer->foregroundColor, this->renderer->backgroundColor);
                break;

            case SlokedTextGraphics::Underscore: {
                Pango::AttrList attrs;
                auto underline = Pango::Attribute::create_attr_underline(Pango::Underline::UNDERLINE_SINGLE);
                attrs.insert(underline);
                this->renderer->textLayout->set_attributes(attrs);
            } break;

            case SlokedTextGraphics::Blink:
            case SlokedTextGraphics::Concealed:
                // Not supported
                break;
        }
    }

    void SlokedCairoTerminal::SetGraphicsMode(SlokedBackgroundGraphics color) {
        std::unique_lock lock(this->renderer->mtx);
        this->updated = true;
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
        std::unique_lock lock(this->renderer->mtx);
        this->updated = true;
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

    bool SlokedCairoTerminal::WaitInput(std::chrono::system_clock::duration timeout) {
        std::unique_lock lock(this->input_mtx);
        if (this->input.empty()) {
            if (timeout == std::chrono::system_clock::duration::zero()) {
                this->input_cv.wait(lock);
            } else {
                this->input_cv.wait_for(lock, timeout);
            }
        }
        return !this->input.empty();
    }

    std::vector<SlokedKeyboardInput> SlokedCairoTerminal::GetInput() {
        std::unique_lock lock(this->input_mtx);
        std::vector<SlokedKeyboardInput> result(this->input.begin(), this->input.end());
        this->input.clear();
        return result;
    }
}