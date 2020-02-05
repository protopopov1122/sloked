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

#include "sloked/screen/sdl/Surface.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedSDLSurface::SlokedSDLSurface(SDL_Surface *surface)
        : surface(surface) {}

    SlokedSDLSurface::SlokedSDLSurface(SDL_Point dim) {
        this->surface = SDL_CreateRGBSurface(0, dim.x, dim.y, 32, 0, 0, 0, 0);
    }

    SlokedSDLSurface::SlokedSDLSurface(SlokedSDLSurface &&surface)
        : surface(surface.surface) {
        surface.surface = nullptr;
    }

    SlokedSDLSurface::~SlokedSDLSurface() {
        if (this->surface != nullptr) {
            SDL_FreeSurface(this->surface);
        }
    }

    SlokedSDLSurface &SlokedSDLSurface::operator=(SlokedSDLSurface &&surface) {
        if (this->surface != nullptr) {
            SDL_FreeSurface(this->surface);
        }
        this->surface = surface.surface;
        surface.surface = nullptr;
        return *this;
    }

    SDL_Surface *SlokedSDLSurface::GetSurface() const {
        return this->surface;
    }

    SDL_Point SlokedSDLSurface::Size() const {
        if (this->surface != nullptr) {
            return {this->surface->w, this->surface->h};
        } else {
            throw SlokedError("SDLSurface: No surface defined");
        }
    }

    Uint32 SlokedSDLSurface::MapColor(SDL_Color color) const {
        if (this->surface != nullptr) {
            return SDL_MapRGBA(this->surface->format, color.r, color.g, color.b, color.a);
        } else {
            throw SlokedError("SDLSurface: No surface defined");
        }
    }

    SDL_Color SlokedSDLSurface::MapColor(Uint32 value) const {
        if (this->surface != nullptr) {
            SDL_Color color;
            SDL_GetRGBA(value, this->surface->format, &color.r, &color.g, &color.b, &color.a);
            return color;
        } else {
            throw SlokedError("SDLSurface: No surface defined");
        }
    }

    void SlokedSDLSurface::Fill(SDL_Rect rect, SDL_Color color) const {
        if (this->surface != nullptr) {
            SDL_FillRect(this->surface, &rect, this->MapColor(std::move(color)));
        } else {
            throw SlokedError("SDLSurface: No surface defined");
        }
    }
}