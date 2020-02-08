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

#ifndef SLOKED_SCREEN_WIDGETS_TEXTPANE_H_
#define SLOKED_SCREEN_WIDGETS_TEXTPANE_H_

#include "sloked/screen/Character.h"
#include "sloked/screen/Point.h"
#include "sloked/screen/Component.h"
#include "sloked/screen/Graphics.h"

namespace sloked {

    class SlokedTextPane {
     public:

        virtual ~SlokedTextPane() = default;
    
        virtual void SetPosition(TextPosition::Line, TextPosition::Column) = 0;
        virtual void MoveUp(TextPosition::Line) = 0;
        virtual void MoveDown(TextPosition::Line) = 0;
        virtual void MoveBackward(TextPosition::Column) = 0;
        virtual void MoveForward(TextPosition::Column) = 0;

        virtual void ShowCursor(bool) = 0;
        virtual void ClearScreen() = 0;
        virtual void ClearArea(TextPosition) = 0;
        virtual SlokedGraphicsPoint::Coordinate GetMaxWidth() = 0;
        virtual TextPosition::Line GetHeight() = 0;
        virtual const SlokedCharacterVisualPreset &GetCharPreset() const = 0;

        virtual void Write(const std::string &) = 0;

        virtual void SetGraphicsMode(SlokedTextGraphics) = 0;
        virtual void SetGraphicsMode(SlokedBackgroundGraphics) = 0;
        virtual void SetGraphicsMode(SlokedForegroundGraphics) = 0;
    };
}

#endif