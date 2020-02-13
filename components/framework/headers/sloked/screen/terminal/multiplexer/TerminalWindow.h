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

#ifndef SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALWINDOW_H_
#define SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALWINDOW_H_

#include "sloked/core/Encoding.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/core/CharPreset.h"
#include "sloked/screen/terminal/multiplexer/BufferedGraphics.h"
#include <functional>

namespace sloked {

    class TerminalWindow : public SlokedTerminal {
     public:
        TerminalWindow(SlokedTerminal &, const Encoding &, const SlokedCharPreset &, const TextPosition &, const TextPosition &);

        void Move(const TextPosition &);
        void Resize(const TextPosition &);
        const TextPosition &GetOffset() const;
        TerminalWindow SubWindow(const TextPosition &, const TextPosition &) const;

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

        void Write(const std::string &) override;

        void SetGraphicsMode(SlokedTextGraphics) override;
        void SetGraphicsMode(SlokedBackgroundGraphics) override;
        void SetGraphicsMode(SlokedForegroundGraphics) override;

        void UpdateDimensions() override;
        void Flush(bool) override;

     private:
        SlokedTerminal &term;
        const Encoding &encoding;
        const SlokedCharPreset &charPreset;
        TextPosition offset;
        TextPosition size;
        Line line;
        Column col;
        std::string buffer;
    };
}

#endif