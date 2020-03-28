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

#ifndef SLOKED_SCREEN_CAIRO_SDL_H_
#define SLOKED_SCREEN_CAIRO_SDL_H_

#include "sloked/screen/cairo/Window.h"
#include "sloked/screen/sdl/Surface.h"
#include "sloked/screen/sdl/Texture.h"
#include "sloked/screen/sdl/Renderer.h"
#include <mutex>

namespace sloked {

    class SlokedSDLCairoSurface {
     public:
        SlokedSDLCairoSurface(int, int);
        int GetWidth() const;
        int GetHeight() const;
        Cairo::RefPtr<Cairo::Surface> GetCairoSurface() const;
        void Resize(int, int);
        std::unique_lock<std::mutex> Lock();
        SlokedSDLTexture MakeTexture(SDL_Renderer *);

     private:
        std::mutex mtx;
        int width;
        int height;
        SlokedSDLSurface sdlSurface;
        Cairo::RefPtr<Cairo::Surface> cairoSurface;
    };

    class SlokedSDLCairoWindow : public SlokedAbstractCairoWindow {
     public:
        SlokedSDLCairoWindow(SlokedScreenManager &, SlokedGraphicsDimensions, const std::string & = "");
        ~SlokedSDLCairoWindow();
        const std::string &GetTitle() const final;
        void SetTitle(const std::string &) final;
        SlokedGraphicsDimensions GetSize() const final;
        bool Resize(SlokedGraphicsDimensions) final;
        std::shared_ptr<SlokedCairoScreenComponent> GetRoot() const final;
        void SetRoot(std::shared_ptr<SlokedCairoScreenComponent>) final;
        void Render() final;
        void Close() final;

     private:
        void PollInput();
        
        SlokedScreenManager &screenMgr;
        SlokedSDLWindow sdlWindow;
        SlokedSDLRenderer sdlRenderer;
        SlokedSDLCairoSurface rootSurface;
        std::shared_ptr<SlokedCairoScreenComponent> root;
        std::atomic_bool resize_request;
        std::pair<int, int> new_size;
        std::string title;
    };
}

#endif