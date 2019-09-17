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

#include "sloked/screen/terminal/components/TextPane.h"

namespace sloked {

    TerminalTextPane::TerminalTextPane(SlokedTerminal &term)
        : term(term) {}

    void TerminalTextPane::SetPosition(Line l, Column c) {
        this->term.SetPosition(l, c);
    }

    void TerminalTextPane::MoveUp(Line l) {
        this->term.MoveUp(l);
    }

    void TerminalTextPane::MoveDown(Line l) {
        this->term.MoveDown(l);
    }

    void TerminalTextPane::MoveBackward(Column c) {
        this->term.MoveBackward(c);
    }

    void TerminalTextPane::MoveForward(Column c) {
        this->term.MoveForward(c);
    }

    void TerminalTextPane::ShowCursor(bool s) {
        this->term.ShowCursor(s);
    }

    void TerminalTextPane::ClearScreen() {
        this->term.ClearScreen();
    }

    void TerminalTextPane::ClearChars(Column c) {
        this->term.ClearChars(c);
    }

    TextPosition::Column TerminalTextPane::GetWidth() {
        return this->term.GetWidth();
    }

    TextPosition::Line TerminalTextPane::GetHeight() {
        return this->term.GetHeight();
    }

    void TerminalTextPane::Write(const std::string &s) {
        this->term.Write(s);
    }

    void TerminalTextPane::SetGraphicsMode(SlokedTextGraphics m) {
        this->term.SetGraphicsMode(m);
    }

    void TerminalTextPane::SetGraphicsMode(SlokedBackgroundGraphics m) {
        this->term.SetGraphicsMode(m);
    }

    void TerminalTextPane::SetGraphicsMode(SlokedForegroundGraphics m) {
        this->term.SetGraphicsMode(m);
    }
}