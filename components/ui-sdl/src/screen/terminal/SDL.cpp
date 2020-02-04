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

    SlokedSDLTerminal::SlokedSDLTerminal(std::unique_ptr<SlokedSDLWindow> window)
        : window(std::move(window)) {
        this->window->SetRenderer([this](SDL_Window *window, SDL_Renderer *renderer) {
            this->RenderSurface(window, renderer);
        });
    }

    void SlokedSDLTerminal::Render() {
        std::unique_lock lock(this->mainSurfaceMtx);
        if (this->mainSurface == nullptr) {
            auto winSize = this->window->Size();
            this->mainSurface = std::make_unique<SlokedSDLSurface>(winSize);
            this->mainSurface->Fill({0, 0, winSize.x, winSize.y}, {255, 255, 255});
        }
    }
    
    void SlokedSDLTerminal::RenderSurface(SDL_Window *, SDL_Renderer *renderer) {
        this->Render();
        std::unique_lock lock(this->mainSurfaceMtx);
        if (this->mainSurface) {
            SlokedSDLTexture texture(renderer, *this->mainSurface);
            SDL_RenderCopy(renderer, texture.GetTexture(), nullptr, nullptr);
        }
    }
}