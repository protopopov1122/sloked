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

#include "sloked/screen/sdl/Font.h"
#include "sloked/core/Encoding.h"
#include "sloked/core/Locale.h"
#include "sloked/core/Error.h"
#include <atomic>

namespace sloked {

    static std::atomic<std::size_t> FondInstances{0};
    static EncodingConverter utf8Converter(SlokedLocale::SystemEncoding(), Encoding::Utf8);

    SlokedSDLFont::SlokedSDLFont(TTF_Font *font)
        : font(font) {
        if (FondInstances++ == 0) {
            TTF_Init();
        }
    }

    SlokedSDLFont::SlokedSDLFont(const std::string &path, int size) {
        if (FondInstances++ == 0) {
            TTF_Init();
        }
        this->font = TTF_OpenFont(path.c_str(), size);
    }
        
    SlokedSDLFont::SlokedSDLFont(SlokedSDLFont &&font)
        : font(font.font) {
        font.font = nullptr;
    }

    SlokedSDLFont::~SlokedSDLFont() {
        if (this->font != nullptr) {
            TTF_CloseFont(this->font);
        }
        if (--FondInstances == 0) {
            TTF_Init();
        }
    }

    SlokedSDLFont &SlokedSDLFont::operator=(SlokedSDLFont &&font) {
        if (this->font != nullptr) {
            TTF_CloseFont(this->font);
        }
        this->font = font.font;
        font.font = nullptr;
        return *this;
    }

    TTF_Font *SlokedSDLFont::GetFont() const {
        return this->font;
    }

    SlokedSDLSurface SlokedSDLFont::RenderSolid(std::string_view text, SDL_Color color) const {
        if (this->font != nullptr) {
            return TTF_RenderUTF8_Solid(this->font, utf8Converter.Convert(text).c_str(), std::move(color));
        } else {
            throw SlokedError("SDLFont: Font not defined");
        }
    }

    SlokedSDLSurface SlokedSDLFont::RenderShaded(std::string_view text, SDL_Color fgcolor, SDL_Color bgcolor) const {
        if (this->font != nullptr) {
            return TTF_RenderUTF8_Shaded(this->font, utf8Converter.Convert(text).c_str(), std::move(fgcolor), std::move(bgcolor));
        } else {
            throw SlokedError("SDLFont: Font not defined");
        }
    }

    SlokedSDLSurface SlokedSDLFont::RenderBlended(std::string_view text, SDL_Color color) const {
        if (this->font != nullptr) {
            return TTF_RenderUTF8_Blended(this->font, utf8Converter.Convert(text).c_str(), std::move(color));
        } else {
            throw SlokedError("SDLFont: Font not defined");
        }
    }

    int SlokedSDLFont::GetStyle() const {
        if (this->font != nullptr) {
            return TTF_GetFontStyle(this->font);
        } else {
            throw SlokedError("SDLFont: Font not defined");
        }
    }
    
    void SlokedSDLFont::SetStyle(int style) const {
        if (this->font != nullptr) {
            TTF_SetFontStyle(this->font, style);
        } else {
            throw SlokedError("SDLFont: Font not defined");
        }
    }

    int SlokedSDLFont::GetOutline() const {
        if (this->font != nullptr) {
            return TTF_GetFontOutline(this->font);
        } else {
            throw SlokedError("SDLFont: Font not defined");
        }
    }

    void SlokedSDLFont::SetOutline(int value) const {
        if (this->font != nullptr) {
            TTF_SetFontOutline(this->font, value);
        } else {
            throw SlokedError("SDLFont: Font not defined");
        }
    }

    int SlokedSDLFont::GetHinting() const {
        if (this->font != nullptr) {
            return TTF_GetFontHinting(this->font);
        } else {
            throw SlokedError("SDLFont: Font not defined");
        }
    }

    void SlokedSDLFont::SetHinting(int value) const {
        if (this->font != nullptr) {
            TTF_SetFontHinting(this->font, value);
        } else {
            throw SlokedError("SDLFont: Font not defined");
        }
    }
}