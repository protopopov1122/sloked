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

#include "sloked/screen/cairo/Surface.h"

namespace sloked {

    SlokedCairoSurface::SlokedCairoSurface(SlokedSDLWindow &window, SlokedSDLRenderer &renderer, SlokedSDLSurface &sdlSurface)
        : surface{nullptr} {
        auto winSize = window.Size();
        SDL_Point rendererSize;
        SDL_GetRendererOutputSize(renderer.GetRenderer(), &rendererSize.x, &rendererSize.y);

        int cairo_x_multiplier = rendererSize.x / winSize.x;
        int cairo_y_multiplier = rendererSize.y / winSize.y;
        
        this->surface = cairo_image_surface_create_for_data((unsigned char *) sdlSurface.GetSurface()->pixels,
            CAIRO_FORMAT_RGB24,
            sdlSurface.GetSurface()->w,
            sdlSurface.GetSurface()->h,
            sdlSurface.GetSurface()->pitch);
        cairo_surface_set_device_scale(this->surface, cairo_x_multiplier, cairo_y_multiplier);
    }
    
    SlokedCairoSurface::SlokedCairoSurface(SlokedCairoSurface &&surface)
        : surface{surface.surface} {
        surface.surface = nullptr;
    }

    SlokedCairoSurface::~SlokedCairoSurface() {
        if (this->surface) {
            cairo_surface_destroy(this->surface);
        }
    }

    SlokedCairoSurface &SlokedCairoSurface::operator=(SlokedCairoSurface &&surface) {
        if (this->surface) {
            cairo_surface_destroy(this->surface);
        }
        this->surface = surface.surface;
        surface.surface = nullptr;
        return *this;
    }

    cairo_surface_t *SlokedCairoSurface::GetSurface() const {
        return this->surface;
    }
}