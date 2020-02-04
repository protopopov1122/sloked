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

#ifndef SLOKED_SCREEN_SDL_TEXTURE_H_
#define SLOKED_SCREEN_SDL_TEXTURE_H_

#include "sloked/screen/sdl/Surface.h"

namespace sloked {

    class SlokedSDLTexture {
     public:
        SlokedSDLTexture(SDL_Texture *);
        SlokedSDLTexture(SDL_Renderer *, const SlokedSDLSurface &);
        SlokedSDLTexture(const SlokedSDLTexture &) = delete;
        SlokedSDLTexture(SlokedSDLTexture &&);
        ~SlokedSDLTexture();

        SlokedSDLTexture &operator=(const SlokedSDLTexture &) = delete;
        SlokedSDLTexture &operator=(SlokedSDLTexture &&);

        SDL_Texture *GetTexture() const;

     private:
        SDL_Texture *texture;
    };
}

#endif