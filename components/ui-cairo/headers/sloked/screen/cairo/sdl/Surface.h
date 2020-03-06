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

#ifndef SLOKED_SCREEN_CAIRO_SDL_SURFACE_H_
#define SLOKED_SCREEN_CAIRO_SDL_SURFACE_H_

#include "sloked/screen/cairo/sdl/Base.h"

namespace sloked {

    class SlokedCairoSDLSurface {
     public:
        SlokedCairoSDLSurface(SDL_Surface * = nullptr);
        SlokedCairoSDLSurface(SDL_Point);
        SlokedCairoSDLSurface(const SlokedCairoSDLSurface &) = delete;
        SlokedCairoSDLSurface(SlokedCairoSDLSurface &&);
        ~SlokedCairoSDLSurface();

        SlokedCairoSDLSurface &operator=(const SlokedCairoSDLSurface &) = delete;
        SlokedCairoSDLSurface &operator=(SlokedCairoSDLSurface &&);

        SDL_Surface *GetSurface() const;
        SDL_Point Size() const;

     private:
        SDL_Surface *surface;
    };
}

#endif