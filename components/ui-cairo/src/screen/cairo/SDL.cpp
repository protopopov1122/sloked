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

#include "sloked/screen/cairo/SDL.h"

namespace sloked {

    SlokedSDLCairoSurface::SlokedSDLCairoSurface(int width, int height)
        : width{width}, height{height}, sdlSurface({width, height}) {
        SDL_LockSurface(this->sdlSurface.GetSurface());
        this->cairoSurface = Cairo::ImageSurface::create(
            (unsigned char *) this->sdlSurface.GetSurface()->pixels,
            Cairo::Format::FORMAT_RGB24,
            this->sdlSurface.GetSurface()->w,
            this->sdlSurface.GetSurface()->h,
            this->sdlSurface.GetSurface()->pitch);
    }

    int SlokedSDLCairoSurface::GetWidth() const {
        return this->width;
    }

    int SlokedSDLCairoSurface::GetHeight() const {
        return this->height;
    }

    Cairo::RefPtr<Cairo::Surface> SlokedSDLCairoSurface::GetCairoSurface() const {
        return this->cairoSurface;
    }

    std::unique_lock<std::mutex> SlokedSDLCairoSurface::Lock() {
        return std::unique_lock<std::mutex>{this->mtx};
    }

    SlokedSDLTexture SlokedSDLCairoSurface::MakeTexture(SDL_Renderer *renderer) {
        auto lock = this->Lock();
        return SlokedSDLTexture(renderer, this->sdlSurface);
    }
}