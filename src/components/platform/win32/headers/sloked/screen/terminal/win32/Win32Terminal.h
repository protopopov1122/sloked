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

#ifndef SLOKED_SCREEN_TERMINAL_WIN32_WIN32TERMINAL_H_
#define SLOKED_SCREEN_TERMINAL_WIN32_WIN32TERMINAL_H_

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <windows.h>

#include "sloked/Base.h"
#include "sloked/screen/terminal/Terminal.h"

namespace sloked {

    class Win32Terminal : public SlokedDuplexTerminal {
     public:
        Win32Terminal();
        Win32Terminal(HANDLE, HANDLE);
        virtual ~Win32Terminal();

        void SetPosition(Line, Column) override;
        void MoveUp(Line) override;
        void MoveDown(Line) override;
        void MoveBackward(Column) override;
        void MoveForward(Column) override;

        void ShowCursor(bool) override;
        void ClearScreen() override;
        void ClearChars(Column) override;
        Column GetWidth() override;
        Line GetHeight() override;

        void Write(std::string_view) override;
        bool WaitInput(
            std::chrono::system_clock::duration =
                std::chrono::system_clock::duration::zero()) override;
        std::vector<SlokedKeyboardInput> GetInput() override;

        void SetGraphicsMode(SlokedTextGraphics) override;
        void SetGraphicsMode(SlokedBackgroundGraphics) override;
        void SetGraphicsMode(SlokedForegroundGraphics) override;

        bool UpdateDimensions() override;
        void Flush(bool) override;

     private:
        struct State;

        std::unique_ptr<State> state;
        bool disable_flush;
    };
}  // namespace sloked

#endif