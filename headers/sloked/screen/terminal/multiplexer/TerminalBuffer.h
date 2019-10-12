/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

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

#ifndef SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALBUFFER_H_
#define SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALBUFFER_H_

#include "sloked/core/Position.h"
#include "sloked/core/Encoding.h"
#include "sloked/core/CharWidth.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/multiplexer/BufferedGraphics.h"
#include <memory>
#include <optional>

namespace sloked {

    class BufferedTerminal : public SlokedTerminal {
     public:
        BufferedTerminal(SlokedTerminal &, const Encoding &, const SlokedCharWidth &);

        void Flush();
        void UpdateSize();
    
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

        void Update() override;
        
     private:
        void dump_buffer(std::u32string_view, std::size_t);

        struct Character {
            bool updated = false;
            std::optional<BufferedGraphicsMode> graphics;
            char32_t value = '\0';
        };

        SlokedTerminal &term;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        bool cls;
        bool show_cursor;
        std::unique_ptr<Character[]> buffer;
        BufferedGraphicsMode graphics;
        Line line;
        Column col;
        Column width;
        Line height;
    };
}

#endif