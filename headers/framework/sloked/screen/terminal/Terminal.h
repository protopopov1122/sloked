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

#ifndef SLOKED_SCREEN_TERMINAL_H_
#define SLOKED_SCREEN_TERMINAL_H_

#include "sloked/Base.h"
#include "sloked/core/Position.h"
#include "sloked/screen/Keyboard.h"
#include "sloked/screen/Graphics.h"
#include <vector>
#include <chrono>

namespace sloked {

    template <typename T, typename ... M>
    struct SlokedTerminalSetGraphicsMode {
        static void Set(T &) {}
    };

    template <typename T, typename A, typename ... M>
    struct SlokedTerminalSetGraphicsMode<T, A, M...> {
        static void Set(T &terminal, A mode, M... modes) {
            terminal.SetGraphicsMode(mode);
            SlokedTerminalSetGraphicsMode<T, M...>::Set(terminal, modes...);
        }
    };

    class SlokedTerminalInputSource {
     public:
        virtual ~SlokedTerminalInputSource() = default;
        virtual bool WaitInput(std::chrono::system_clock::duration = std::chrono::system_clock::duration::zero()) = 0;
        virtual std::vector<SlokedKeyboardInput> GetInput() = 0;
    };

    class SlokedTerminal {
     public:
        using Line = TextPosition::Line;
        using Column = TextPosition::Column;
        using Text = SlokedTextGraphics;
        using Foreground = SlokedForegroundGraphics;
        using Background = SlokedBackgroundGraphics;

        virtual ~SlokedTerminal() = default;
    
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

        virtual void UpdateDimensions() {}
        virtual void Flush(bool) {}
        virtual void RenderFrame() {}
    };

    class SlokedDuplexTerminal : public SlokedTerminal, public SlokedTerminalInputSource {};
}

#endif