/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/screen/sdl/Renderer.h"

#include "sloked/core/Error.h"

namespace sloked {

    SlokedSDLRenderer::SlokedSDLRenderer(SDL_Renderer *renderer)
        : renderer{renderer} {}

    SlokedSDLRenderer::SlokedSDLRenderer(const SlokedSDLWindow &window,
                                         Uint32 flags)
        : renderer{nullptr} {
        this->renderer = SDL_CreateRenderer(window.GetWindow(), -1, flags);
        if (this->renderer == nullptr) {
            throw SlokedError("SDLRenderer: Error creating renderer");
        }
    }

    SlokedSDLRenderer::SlokedSDLRenderer(SlokedSDLRenderer &&renderer)
        : renderer{renderer.renderer} {
        renderer.renderer = nullptr;
    }

    SlokedSDLRenderer::~SlokedSDLRenderer() {
        if (this->renderer) {
            SDL_DestroyRenderer(this->renderer);
        }
    }

    SlokedSDLRenderer &SlokedSDLRenderer::operator=(
        SlokedSDLRenderer &&renderer) {
        if (this->renderer) {
            SDL_DestroyRenderer(this->renderer);
        }
        this->renderer = renderer.renderer;
        renderer.renderer = nullptr;
        return *this;
    }

    SDL_Renderer *SlokedSDLRenderer::GetRenderer() const {
        return this->renderer;
    }
}  // namespace sloked