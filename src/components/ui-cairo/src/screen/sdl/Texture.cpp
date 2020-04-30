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

#include "sloked/screen/sdl/Texture.h"

namespace sloked {

    SlokedSDLTexture::SlokedSDLTexture(SDL_Texture *texture)
        : texture(texture) {}

    SlokedSDLTexture::SlokedSDLTexture(SDL_Renderer *renderer,
                                       const SlokedSDLSurface &surface)
        : texture(
              SDL_CreateTextureFromSurface(renderer, surface.GetSurface())) {}

    SlokedSDLTexture::SlokedSDLTexture(SlokedSDLTexture &&texture)
        : texture(texture.texture) {
        texture.texture = nullptr;
    }

    SlokedSDLTexture::~SlokedSDLTexture() {
        if (this->texture != nullptr) {
            SDL_DestroyTexture(this->texture);
        }
    }

    SlokedSDLTexture &SlokedSDLTexture::operator=(SlokedSDLTexture &&texture) {
        if (this->texture != nullptr) {
            SDL_DestroyTexture(this->texture);
        }
        this->texture = texture.texture;
        texture.texture = nullptr;
        return *this;
    }

    SDL_Texture *SlokedSDLTexture::GetTexture() const {
        return this->texture;
    }
}  // namespace sloked