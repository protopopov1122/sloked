/*
  SPDX-License-Identifier: LGPL-3.0-or-later

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_SCREEN_WIDGETS_TEXTPANE_H_
#define SLOKED_SCREEN_WIDGETS_TEXTPANE_H_

#include "sloked/Base.h"
#include "sloked/core/Position.h"
#include "sloked/screen/Component.h"
#include "sloked/screen/Graphics.h"

namespace sloked {

    class SlokedTextPane {
     public:
        using Line = TextPosition::Line;
        using Column = TextPosition::Column;

        virtual ~SlokedTextPane() = default;
    
        virtual void SetPosition(Line, Column) = 0;
        virtual void MoveUp(Line) = 0;
        virtual void MoveDown(Line) = 0;
        virtual void MoveBackward(Column) = 0;
        virtual void MoveForward(Column) = 0;

        virtual void ShowCursor(bool) = 0;
        virtual void ClearScreen() = 0;
        virtual void ClearChars(Column) = 0;
        virtual Column GetWidth() = 0;
        virtual Line GetHeight() = 0;

        virtual void Write(const std::string &) = 0;

        virtual void SetGraphicsMode(SlokedTextGraphics) = 0;
        virtual void SetGraphicsMode(SlokedBackgroundGraphics) = 0;
        virtual void SetGraphicsMode(SlokedForegroundGraphics) = 0;
    };
}

#endif