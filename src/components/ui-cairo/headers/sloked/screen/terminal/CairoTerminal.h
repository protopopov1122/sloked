/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_SCREEN_TERMINAL_CAIROTERMINAL_H_
#define SLOKED_SCREEN_TERMINAL_CAIROTERMINAL_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

#include "sloked/core/Event.h"
#include "sloked/core/LRU.h"
#include "sloked/core/RingBuffer.h"
#include "sloked/screen/Point.h"
#include "sloked/screen/Size.h"
#include "sloked/screen/cairo/Component.h"
#include "sloked/screen/cairo/Window.h"
#include "sloked/screen/graphics/Terminal.h"
#include "sloked/screen/pango/Base.h"

namespace sloked {

    class SlokedCairoTerminal : public SlokedGraphicalTerminal,
                                public SlokedCairoScreenComponent {
     public:
        SlokedCairoTerminal(const std::string &, const Mode &);

        const std::string &GetFont() const final;
        const Mode &GetDefaultMode() const final;
        void SetDefaultMode(const Mode &) final;
        SlokedScreenSize &GetTerminalSize() final;
        const Encoding &GetEncoding() const final;

        bool CheckUpdates() final;
        void ProcessInput(std::vector<SlokedKeyboardInput>) final;
        void SetTarget(const Cairo::RefPtr<Cairo::Surface> &, Dimensions) final;
        Dimensions GetSize() const final;

        void SetPosition(Line, Column) final;
        void MoveUp(Line) final;
        void MoveDown(Line) final;
        void MoveBackward(Column) final;
        void MoveForward(Column) final;

        void ShowCursor(bool) final;
        void ClearScreen() final;
        void ClearChars(Column) final;
        Column GetWidth() final;
        Line GetHeight() final;

        void Write(std::string_view) final;

        void SetGraphicsMode(SlokedTextGraphics) final;
        void SetGraphicsMode(SlokedBackgroundGraphics) final;
        void SetGraphicsMode(SlokedForegroundGraphics) final;

        bool WaitInput(std::chrono::system_clock::duration =
                           std::chrono::system_clock::duration::zero()) final;
        std::vector<SlokedKeyboardInput> GetInput() final;

        bool UpdateDimensions() final;

     private:
        struct Renderer {
            Renderer(const std::string &);
            void SetTarget(const Cairo::RefPtr<Cairo::Surface> &, Dimensions);
            bool IsValid() const;
            void Apply(const Mode &);

            Cairo::RefPtr<Cairo::Surface> surface;
            Cairo::RefPtr<Cairo::Context> context;
            Glib::RefPtr<Pango::FontMap> fontMap;
            Glib::RefPtr<Pango::Layout> textLayout;
            Pango::FontDescription font;
            Cairo::RefPtr<Cairo::SolidPattern> backgroundColor;
            Dimensions surfaceSize;
            Dimensions glyphSize;
            std::mutex mtx;
        };

        class Size : public SlokedScreenSize {
         public:
            Size(SlokedCairoTerminal &);
            TextPosition GetScreenSize() const final;
            std::function<void()> Listen(Listener) final;
            void Notify();

         private:
            SlokedCairoTerminal &terminal;
            SlokedEventEmitter<TextPosition> emitter;
        };

        struct Input {
            static constexpr std::size_t BufferSize = 4096;

            std::mutex mtx;
            std::condition_variable cv;
            SlokedRingBuffer<SlokedKeyboardInput> content{BufferSize};
        };

        struct CacheEntry {
            std::string text;
            Mode mode;

            bool operator<(const CacheEntry &) const;
        };

        void DrawText(std::string_view);
        void FlipCursor();

        Renderer renderer;
        Size screenSize;
        Input input;
        std::atomic_bool updated;
        const std::string font;
        TextPosition size;
        TextPosition cursor;
        bool showCursor;
        Mode defaultMode;
        Mode mode;
        SlokedLRUCache<CacheEntry, Cairo::RefPtr<Cairo::ImageSurface>> cache;
        std::chrono::system_clock::time_point lastResize;
    };

    class SlokedCairoTerminalWindow : public SlokedGraphicalTerminalWindow {
     public:
        SlokedCairoTerminalWindow(std::unique_ptr<SlokedAbstractCairoWindow>,
                                  std::unique_ptr<SlokedCairoTerminal>);

        const std::string &GetTitle() const final;
        void SetTitle(const std::string &) final;
        SlokedGraphicsDimensions GetSize() const final;
        bool Resize(SlokedGraphicsDimensions) final;
        void Close() final;
        SlokedGraphicalTerminal &GetTerminal() final;

     private:
        std::unique_ptr<SlokedAbstractCairoWindow> window;
        std::shared_ptr<SlokedCairoTerminal> terminal;
    };
}  // namespace sloked

#endif