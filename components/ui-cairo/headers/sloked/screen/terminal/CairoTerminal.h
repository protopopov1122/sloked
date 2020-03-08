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

#ifndef SLOKED_SCREEN_TERMINAL_CAIROTERMINAL_H_
#define SLOKED_SCREEN_TERMINAL_CAIROTERMINAL_H_

#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/Point.h"
#include "sloked/screen/cairo/Base.h"
#include "sloked/screen/pango/Base.h"
#include <mutex>
#include <memory>

namespace sloked {

    class SlokedCairoTerminal : public SlokedTerminal {
     public:
        struct Dimensions {
            int x;
            int y;
        };
        using Line = TextPosition::Line;
        using Column = TextPosition::Column;

        SlokedCairoTerminal(Cairo::RefPtr<Cairo::Surface>, Dimensions);
        ~SlokedCairoTerminal();

        void SetPosition(Line, Column) final;
        void MoveUp(Line) final;
        void MoveDown(Line) final;
        void MoveBackward(Column) final;
        void MoveForward(Column) final;

        void ShowCursor(bool) final;
        void ClearScreen() final;
        void ClearChars(Column) final;
        Column GetWidth() final;
        Line GetHeight() final;

        void Write(std::string_view) final;

        void SetGraphicsMode(SlokedTextGraphics) final;
        void SetGraphicsMode(SlokedBackgroundGraphics) final;
        void SetGraphicsMode(SlokedForegroundGraphics) final;


     private:
        struct Renderer;

        std::unique_ptr<Renderer> renderer;
        TextPosition size;
        TextPosition cursor;
        bool showCursor;
        Dimensions surfaceSize;
        Dimensions glyphSize;
    };
}

#endif