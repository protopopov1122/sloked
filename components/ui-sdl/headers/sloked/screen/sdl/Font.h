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

#ifndef SLOKED_SCREEN_SDL_FONT_H_
#define SLOKED_SCREEN_SDL_FONT_H_

#include "sloked/screen/sdl/Surface.h"
#include <string>

namespace sloked {

    class SlokedSDLFont {
     public:
        SlokedSDLFont(TTF_Font *);
        SlokedSDLFont(const std::string &, int);
        SlokedSDLFont(const SlokedSDLFont &) = delete;
        SlokedSDLFont(SlokedSDLFont &&);
        ~SlokedSDLFont();

        SlokedSDLFont &operator=(const SlokedSDLFont &) = delete;
        SlokedSDLFont &operator=(SlokedSDLFont &&);

        TTF_Font *GetFont() const;
        SlokedSDLSurface RenderSolid(std::string_view, SDL_Color) const;
        SlokedSDLSurface RenderShaded(std::string_view, SDL_Color, SDL_Color) const;
        SlokedSDLSurface RenderBlended(std::string_view, SDL_Color) const;
        int GetStyle() const;
        void SetStyle(int = TTF_STYLE_NORMAL) const;
        int GetOutline() const;
        void SetOutline(int) const;
        int GetHinting() const;
        void SetHinting(int) const;
        bool GetKerning() const;
        void SetKerning(bool) const;

     private:
        TTF_Font *font;
    };
}

#endif