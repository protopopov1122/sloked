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

#ifndef SLOKED_SCREEN_SDL_RENDERER_H_
#define SLOKED_SCREEN_SDL_RENDERER_H_

#include "sloked/screen/sdl/Window.h"

namespace sloked {

    class SlokedSDLRenderer {
     public:
        SlokedSDLRenderer(SDL_Renderer *);
        SlokedSDLRenderer(const SlokedSDLWindow &, Uint32 = 0);
        SlokedSDLRenderer(const SlokedSDLRenderer &) = delete;
        SlokedSDLRenderer(SlokedSDLRenderer &&);
        ~SlokedSDLRenderer();

        SlokedSDLRenderer &operator=(const SlokedSDLRenderer &) = delete;
        SlokedSDLRenderer &operator=(SlokedSDLRenderer &&);

        SDL_Renderer *GetRenderer();

     private:
        SDL_Renderer *renderer;
    };
}

#endif