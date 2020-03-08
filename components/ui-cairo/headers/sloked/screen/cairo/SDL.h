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

#include "sloked/screen/cairo/Base.h"
#include "sloked/screen/sdl/Surface.h"
#include "sloked/screen/sdl/Texture.h"
#include <mutex>

namespace sloked {

    class SlokedSDLCairoSurface {
     public:
        SlokedSDLCairoSurface(int, int);
        int GetWidth() const;
        int GetHeight() const;
        Cairo::RefPtr<Cairo::Surface> GetCairoSurface() const;
        std::unique_lock<std::mutex> Lock();
        SlokedSDLTexture MakeTexture(SDL_Renderer *);

     private:
        std::mutex mtx;
        int width;
        int height;
        SlokedSDLSurface sdlSurface;
        Cairo::RefPtr<Cairo::Surface> cairoSurface;
    };
}

#endif