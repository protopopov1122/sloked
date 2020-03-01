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

#include "sloked/screen/terminal/SDL.h"

namespace sloked {

    SlokedSDLTerminal::SlokedSDLTerminal()
        : text{} {}

    void SlokedSDLTerminal::PollEvents(SlokedSDLEventQueue &events) {
        while (events.HasEvents()) {
            auto event = events.NextEvent();
            if (event.type == SDL_TEXTINPUT) {
                this->text.append(event.text.text);
            }
        }
    }

    void SlokedSDLTerminal::Render(SlokedSDLSurface &surface) {
        surface.Fill({0, 0, surface.Size().x, surface.Size().y}, {255, 255, 255, 0});
        static SlokedSDLFont font("/usr/share/fonts/TTF/DejaVuSansMono.ttf", 12);
        SlokedSDLSurface text = font.RenderShaded(this->text.c_str(), {0, 0, 0, 0}, {255, 255, 255, 0});
        SDL_BlitSurface(text.GetSurface(), nullptr, surface.GetSurface(), nullptr);
    }
}