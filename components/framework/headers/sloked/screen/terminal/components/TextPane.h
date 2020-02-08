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

#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_TEXTPANE_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_TEXTPANE_H_

#include "sloked/Base.h"
#include "sloked/screen/widgets/TextPane.h"
#include "sloked/screen/terminal/Terminal.h"
#include <memory>

namespace sloked {

    class TerminalTextPane : public SlokedTextPane {
     public:
        TerminalTextPane(SlokedTerminal &);
        ~TerminalTextPane();
    
        void SetPosition(TextPosition::Line, TextPosition::Column) final;
        void MoveUp(TextPosition::Line) final;
        void MoveDown(TextPosition::Line) final;
        void MoveBackward(TextPosition::Column) final;
        void MoveForward(TextPosition::Column) final;

        void ShowCursor(bool) final;
        void ClearScreen() final;
        void ClearArea(TextPosition) final;
        SlokedGraphicsPoint::Coordinate GetMaxWidth() final;
        TextPosition::Line GetHeight() final;
        const SlokedCharacterVisualPreset &GetCharPreset() const final;

        void Write(const std::string &) final;

        void SetGraphicsMode(SlokedTextGraphics) final;
        void SetGraphicsMode(SlokedBackgroundGraphics) final;
        void SetGraphicsMode(SlokedForegroundGraphics) final;
        
     private:
        class TerminalVisualPreset;

        SlokedTerminal &term;
        std::unique_ptr<TerminalVisualPreset> visualPreset;
    };
}

#endif