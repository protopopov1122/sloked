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
#include <limits>

namespace sloked {

    template <typename T = guint16>
    constexpr T MapDoubleToInt(double value) {
        return static_cast<T>(value * std::numeric_limits<T>::max());
    }

    void SetForegroundAttribute(Pango::AttrList &attrs, const Cairo::RefPtr<Cairo::SolidPattern> color) {
        double r, g, b, a;
        color->get_rgba(r, g, b, a);
        auto fgColor = Pango::Attribute::create_attr_foreground(MapDoubleToInt(r), MapDoubleToInt(g), MapDoubleToInt(b));
        attrs.insert(fgColor);
    }

    void SetBackgroundAttribute(Pango::AttrList &attrs, const Cairo::RefPtr<Cairo::SolidPattern> color) {
        double r, g, b, a;
        color->get_rgba(r, g, b, a);
        auto bgColor = Pango::Attribute::create_attr_background(MapDoubleToInt(r), MapDoubleToInt(g), MapDoubleToInt(b));
        attrs.insert(bgColor); 
    }

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

    SlokedCairoTerminal::Renderer::Renderer(const std::string &fontDescr)
        : surface(nullptr),
            context(nullptr),
            fontMap(Glib::wrap(pango_cairo_font_map_get_default())),
            textLayout(nullptr),
            font(fontDescr),
            backgroundColor(CairoColors::White),
            surfaceSize{0, 0},
            glyphSize{0, 0} {}

    void SlokedCairoTerminal::Renderer::SetTarget(const Cairo::RefPtr<Cairo::Surface> &targetSurface, Dimensions dim) {
        auto newContext = Cairo::Context::create(targetSurface);
        auto newTextLayout = Pango::Layout::create(newContext);
        if (this->textLayout) {
            newTextLayout->set_text(this->textLayout->get_text());
            newTextLayout->set_font_description(this->textLayout->get_font_description());
            auto attrs = this->textLayout->get_attributes();
            newTextLayout->set_attributes(attrs);
        } else {
            newTextLayout->set_font_description(this->font);
        }
        this->textLayout = newTextLayout;
        this->context = newContext;
        this->surface = targetSurface;

        this->surfaceSize = dim;
        this->textLayout->set_text("A");
        this->textLayout->get_pixel_size(this->glyphSize.x, this->glyphSize.y);
    }

    bool SlokedCairoTerminal::Renderer::IsValid() const {
        return this->context.operator bool();
    }

    void SlokedCairoTerminal::Renderer::Apply(const TerminalMode &mode) {
        Pango::AttrList attrs;
        if (mode.bold) {
            auto bold = Pango::Attribute::create_attr_weight(Pango::Weight::WEIGHT_BOLD);
            attrs.insert(bold);
        }
        if (mode.underscore) {
            auto underline = Pango::Attribute::create_attr_underline(Pango::Underline::UNDERLINE_SINGLE);
            attrs.insert(underline);
        }
        switch (mode.background) {
            case SlokedBackgroundGraphics::Black:
                this->backgroundColor = CairoColors::Black;
                break;

            case SlokedBackgroundGraphics::Red:
                this->backgroundColor = CairoColors::Red;
                break;

            case SlokedBackgroundGraphics::Green:
                this->backgroundColor = CairoColors::Green;
                break;

            case SlokedBackgroundGraphics::Yellow:
                this->backgroundColor = CairoColors::Black;
                break;

            case SlokedBackgroundGraphics::Blue:
                this->backgroundColor = CairoColors::Blue;
                break;

            case SlokedBackgroundGraphics::Magenta:
                this->backgroundColor = CairoColors::Magenta;
                break;

            case SlokedBackgroundGraphics::Cyan:
                this->backgroundColor = CairoColors::Cyan;
                break;

            case SlokedBackgroundGraphics::White:
                this->backgroundColor = CairoColors::White;
                break;
        }
        SetBackgroundAttribute(attrs, this->backgroundColor);
        Cairo::RefPtr<Cairo::SolidPattern> foregroundColor;
        switch (mode.foreground) {
            case SlokedForegroundGraphics::Black:
                foregroundColor = CairoColors::Black;
                break;

            case SlokedForegroundGraphics::Red:
                foregroundColor = CairoColors::Red;
                break;

            case SlokedForegroundGraphics::Green:
                foregroundColor = CairoColors::Green;
                break;

            case SlokedForegroundGraphics::Yellow:
                foregroundColor = CairoColors::Black;
                break;

            case SlokedForegroundGraphics::Blue:
                foregroundColor = CairoColors::Blue;
                break;

            case SlokedForegroundGraphics::Magenta:
                foregroundColor = CairoColors::Magenta;
                break;

            case SlokedForegroundGraphics::Cyan:
                foregroundColor = CairoColors::Cyan;
                break;

            case SlokedForegroundGraphics::White:
                foregroundColor = CairoColors::White;
                break;
        }
        SetForegroundAttribute(attrs, foregroundColor);
        this->textLayout->set_attributes(attrs);
    }

    SlokedCairoTerminal::Size::Size(SlokedCairoTerminal &terminal)
        : terminal(terminal) {}

    TextPosition SlokedCairoTerminal::Size::GetScreenSize() const {
        return {
            terminal.GetHeight(),
            terminal.GetWidth()
        };
    }
    
    std::function<void()> SlokedCairoTerminal::Size::Listen(Listener listener) {
        return this->emitter.Listen(std::move(listener));
    }

    void SlokedCairoTerminal::Size::Notify() {
        this->emitter.Emit(this->GetScreenSize());
    }

    bool SlokedCairoTerminal::CacheEntry::operator<(const CacheEntry &other) const {
        return this->text < other.text ||
            (this->text == other.text && this->mode.background < other.mode.background) ||
            (this->text == other.text && this->mode.background == other.mode.background &&
                this->mode.foreground < other.mode.foreground) ||
            (this->text == other.text && this->mode.background == other.mode.background &&
                this->mode.foreground == other.mode.foreground && this->mode.bold < other.mode.bold) ||
            (this->text == other.text && this->mode.background == other.mode.background &&
                this->mode.foreground == other.mode.foreground && this->mode.bold == other.mode.bold &&
                this->mode.underscore < other.mode.underscore);
    }

    SlokedCairoTerminal::SlokedCairoTerminal(const std::string &font)
        : renderer(font), screenSize(*this),
          updated{true}, size{0, 0}, cursor{0, 0}, showCursor{true}, mode{},
          cache{1024} {}

    SlokedCairoTerminal::~SlokedCairoTerminal() = default;

    SlokedScreenSize &SlokedCairoTerminal::GetTerminalSize() {
        return this->screenSize;
    }

    bool SlokedCairoTerminal::CheckUpdates() {
        return this->updated.exchange(false);
    }

    void SlokedCairoTerminal::ProcessInput(std::vector<SlokedKeyboardInput> events) {
        std::unique_lock lock(this->input.mtx);
        if (this->input.content.available() < events.size()) {
            this->input.content.pop_front(events.size() - this->input.content.available());
        }
        this->input.content.insert(events.begin(), events.end());
        this->input.cv.notify_all();
    }

    void SlokedCairoTerminal::SetTarget(const Cairo::RefPtr<Cairo::Surface> &targetSurface, Dimensions dim) {
        std::unique_lock lock(this->renderer.mtx);
        this->renderer.SetTarget(targetSurface, dim);
        this->size = {
            static_cast<TextPosition::Line>(this->renderer.surfaceSize.y / this->renderer.glyphSize.y),
            static_cast<TextPosition::Column>(this->renderer.surfaceSize.x / this->renderer.glyphSize.x)
        };
        lock.unlock();
        this->ClearScreen();
        this->FlipCursor();
        this->screenSize.Notify();
    }

    SlokedCairoTerminal::Dimensions SlokedCairoTerminal::GetSize() const {
        return this->renderer.surfaceSize;
    }
    
    void SlokedCairoTerminal::SetPosition(Line l, Column c) {
        this->FlipCursor();
        this->updated = true;
        this->cursor = {
            std::min(l, this->size.line - 1),
            std::min(c, this->size.column - 1)
        };
        this->FlipCursor();
    }

    void SlokedCairoTerminal::MoveUp(Line l) {
        this->FlipCursor();
        this->updated = true;
        this->cursor.line -= std::min(this->cursor.line, l);
        this->FlipCursor();
    }

    void SlokedCairoTerminal::MoveDown(Line l) {
        this->FlipCursor();
        this->updated = true;
        this->cursor.line = std::max(this->size.line - 1, this->cursor.line + l);
        this->FlipCursor();
    }

    void SlokedCairoTerminal::MoveBackward(Column c) {
        this->FlipCursor();
        this->updated = true;
        this->cursor.column -= std::min(this->cursor.column, c);
        this->FlipCursor();
    }

    void SlokedCairoTerminal::MoveForward(Column c) {
        this->FlipCursor();
        this->updated = true;
        this->cursor.column = std::max(this->size.column - 1, this->cursor.column + c);
        this->FlipCursor();
    }

    void SlokedCairoTerminal::ShowCursor(bool show) {
        this->FlipCursor();
        this->updated = true;
        this->showCursor = show;
        this->FlipCursor();
    }

    void SlokedCairoTerminal::ClearScreen() {
        std::unique_lock lock(this->renderer.mtx);
        if (this->renderer.IsValid()) {
            this->updated = true;
            this->renderer.context->set_source(this->renderer.backgroundColor);
            this->renderer.context->rectangle(0, 0, this->renderer.surfaceSize.x, this->renderer.surfaceSize.y);
            this->renderer.context->fill();
            this->FlipCursor();
        }
    }

    void SlokedCairoTerminal::ClearChars(Column col) {
        std::unique_lock lock(this->renderer.mtx);
        if (this->renderer.IsValid()) {
            this->FlipCursor();
            this->updated = true;
            this->renderer.context->set_source(this->renderer.backgroundColor);
            this->renderer.context->rectangle(this->cursor.column * this->renderer.glyphSize.x,
                this->cursor.line * this->renderer.glyphSize.y,
                std::min(col, this->size.column - this->cursor.column) * this->renderer.glyphSize.x,
                this->renderer.glyphSize.y);
            this->renderer.context->fill();
            this->FlipCursor();
        }
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
            if (currentLength + it.length >= size.column - cursor.column) {
                callback(content.substr(start, currentLength));
                if (cursor.line + 1 == size.line) {
                    return false;
                }
                start = it.start;
                currentLength = 0;
                cursor.column = 0;
                cursor.line++;
            }
            currentLength += it.length;
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
        std::unique_lock lock(this->renderer.mtx);
        if (this->renderer.IsValid()) {
            this->FlipCursor();
            this->updated = true;
            const Encoding &encoding = SlokedLocale::SystemEncoding();
            auto newline = NewLine::LF(encoding);
            std::vector<std::string_view> lines;
            SplitNewLines(lines, content, *newline);
            for (const auto &line : lines) {
                if (!ShowLine([&](auto lineContent) {
                    if (lineContent.empty()) {
                        return;
                    }
                    this->DrawText(lineContent);
                }, line, encoding, this->cursor, this->size)) {
                    break;
                }
            }
            this->FlipCursor();
        }
    }

    void SlokedCairoTerminal::SetGraphicsMode(SlokedTextGraphics mode) {
        std::unique_lock lock(this->renderer.mtx);
        this->updated = true;
        switch (mode) {
            case SlokedTextGraphics::Off:
                this->mode = {};
                if (this->renderer.IsValid()) {
                    this->renderer.Apply(this->mode);
                }
                break;

            case SlokedTextGraphics::Bold:
                this->mode.bold = true;
                if (this->renderer.IsValid()) {
                    this->renderer.Apply(this->mode);
                }
                break;

            case SlokedTextGraphics::Reverse:
                std::swap(this->mode.background, this->mode.background);
                if (this->renderer.IsValid()) {
                    this->renderer.Apply(this->mode);
                }
                break;

            case SlokedTextGraphics::Underscore: {
                this->mode.underscore = true;
                if (this->renderer.IsValid()) {
                    this->renderer.Apply(this->mode);
                }
            } break;

            case SlokedTextGraphics::Blink:
            case SlokedTextGraphics::Concealed:
                // Not supported
                break;
        }
    }

    void SlokedCairoTerminal::SetGraphicsMode(SlokedBackgroundGraphics color) {
        std::unique_lock lock(this->renderer.mtx);
        this->updated = true;
        this->mode.background = color;
        this->renderer.Apply(this->mode);
    }

    void SlokedCairoTerminal::SetGraphicsMode(SlokedForegroundGraphics color) {
        std::unique_lock lock(this->renderer.mtx);
        this->updated = true;
        this->mode.foreground = color;
        this->renderer.Apply(this->mode);
    }

    bool SlokedCairoTerminal::WaitInput(std::chrono::system_clock::duration timeout) {
        std::unique_lock lock(this->input.mtx);
        if (this->input.content.empty()) {
            if (timeout == std::chrono::system_clock::duration::zero()) {
                this->input.cv.wait(lock);
            } else {
                this->input.cv.wait_for(lock, timeout);
            }
        }
        return !this->input.content.empty();
    }

    std::vector<SlokedKeyboardInput> SlokedCairoTerminal::GetInput() {
        std::unique_lock lock(this->input.mtx);
        std::vector<SlokedKeyboardInput> result(this->input.content.begin(), this->input.content.end());
        this->input.content.clear();
        return result;
    }
    
    void SlokedCairoTerminal::DrawText(std::string_view content) {
        CacheEntry key{std::string{content}, this->mode};
        if (this->cache.Has(key)) {
            auto prerender = this->cache.At(key);            
            this->renderer.context->set_source(prerender, static_cast<double>(this->cursor.column * this->renderer.glyphSize.x),
                static_cast<double>(this->cursor.line * this->renderer.glyphSize.y));
            this->renderer.context->rectangle(static_cast<double>(this->cursor.column * this->renderer.glyphSize.x),
                static_cast<double>(this->cursor.line * this->renderer.glyphSize.y), prerender->get_width(), prerender->get_height());
            this->renderer.context->fill();

        } else {
            int width, height;
            this->renderer.textLayout->set_text(key.text);
            this->renderer.textLayout->get_pixel_size(width, height);
            this->renderer.context->move_to(this->cursor.column * this->renderer.glyphSize.x, this->cursor.line * this->renderer.glyphSize.y);
            this->renderer.textLayout->show_in_cairo_context(this->renderer.context);

            auto prerender = Cairo::ImageSurface::create(Cairo::Format::FORMAT_RGB24,
                width, height);
            auto prerenderContext = Cairo::Context::create(prerender);
            prerenderContext->set_source(this->renderer.surface, -static_cast<double>(this->cursor.column * this->renderer.glyphSize.x),
                -static_cast<double>(this->cursor.line * this->renderer.glyphSize.y));
            prerenderContext->rectangle(0, 0, width, height);
            prerenderContext->fill();
            this->cache.Insert(key, prerender);
        }
    }

    void SlokedCairoTerminal::FlipCursor() {
        if (this->showCursor) {
            this->renderer.context->set_source_rgb(1.0, 1.0, 1.0);
            this->renderer.context->set_operator(static_cast<Cairo::Operator>(CAIRO_OPERATOR_DIFFERENCE));
            this->renderer.context->rectangle(this->cursor.column * this->renderer.glyphSize.x, this->cursor.line * this->renderer.glyphSize.y, this->renderer.glyphSize.x, this->renderer.glyphSize.y);
            this->renderer.context->fill();
            this->renderer.context->set_operator(Cairo::Operator::OPERATOR_SOURCE);
        }
    }
}