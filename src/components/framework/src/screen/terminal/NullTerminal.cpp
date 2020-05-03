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

#include "sloked/screen/terminal/NullTerminal.h"
#include <thread>

namespace sloked {

    SlokedNullTerminal::SlokedNullTerminal(Line height, Column width)
        : height(height), width(width) {}

    void SlokedNullTerminal::SetPosition(Line, Column) {}
    void SlokedNullTerminal::MoveUp(Line) {}
    void SlokedNullTerminal::MoveDown(Line) {}
    void SlokedNullTerminal::MoveBackward(Column) {}
    void SlokedNullTerminal::MoveForward(Column) {}

    void SlokedNullTerminal::ShowCursor(bool) {}
    void SlokedNullTerminal::ClearScreen() {}
    void SlokedNullTerminal::ClearChars(Column) {}

    SlokedNullTerminal::Column SlokedNullTerminal::GetWidth() {
        return this->width;
    }

    SlokedNullTerminal::Line SlokedNullTerminal::GetHeight() {
        return this->height;
    }

    void SlokedNullTerminal::Write(std::string_view) {}

    void SlokedNullTerminal::SetGraphicsMode(SlokedTextGraphics) {}
    void SlokedNullTerminal::SetGraphicsMode(SlokedBackgroundGraphics) {}
    void SlokedNullTerminal::SetGraphicsMode(SlokedForegroundGraphics) {}

    bool SlokedNullTerminal::WaitInput(std::chrono::system_clock::duration dur) {
        std::this_thread::sleep_for(dur);
        return false;
    }

    std::vector<SlokedKeyboardInput> SlokedNullTerminal::GetInput() {
        return {};
    }
}