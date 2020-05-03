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

#ifndef SLOKED_SCREEN_NULLTERMINAL_H_
#define SLOKED_SCREEN_NULLTERMINAL_H_

#include "sloked/screen/terminal/Terminal.h"

namespace sloked {

    class SlokedNullTerminal : public SlokedDuplexTerminal {
     public:
        SlokedNullTerminal(Line, Column);

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

        bool WaitInput(std::chrono::system_clock::duration =
                           std::chrono::system_clock::duration::zero()) final;
        std::vector<SlokedKeyboardInput> GetInput() final;

     private:
        Line height;
        Column width;
    };
}  // namespace sloked

#endif