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

#include "sloked/screen/terminal/components/TextPane.h"

namespace sloked {

    class TerminalTextPane::TerminalVisualPreset : public SlokedFontProperties {
     public:
        SlokedGraphicsPoint::Coordinate GetWidth(char32_t) const final {
            return 1;
        }

        SlokedGraphicsPoint::Coordinate GetHeight() const final {
            return 1;
        }
    };

    TerminalTextPane::TerminalTextPane(SlokedTerminal &term)
        : term(term), fontProperties(std::make_unique<TerminalVisualPreset>()) {}

    TerminalTextPane::~TerminalTextPane() = default;

    void TerminalTextPane::SetPosition(TextPosition::Line line, TextPosition::Column column) {
        this->term.SetPosition(line, column);
    }

    void TerminalTextPane::MoveUp(TextPosition::Line line) {
        this->term.MoveUp(line);
    }

    void TerminalTextPane::MoveDown(TextPosition::Line line) {
        this->term.MoveDown(line);
    }

    void TerminalTextPane::MoveBackward(TextPosition::Column column) {
        this->term.MoveBackward(column);
    }

    void TerminalTextPane::MoveForward(TextPosition::Column column) {
        this->term.MoveForward(column);
    }

    void TerminalTextPane::ShowCursor(bool s) {
        this->term.ShowCursor(s);
    }

    void TerminalTextPane::ClearScreen() {
        this->term.ClearScreen();
    }

    void TerminalTextPane::ClearArea(TextPosition range) {
        for (SlokedGraphicsPoint::Coordinate line = 0; line < range.line; line++) {
            this->term.MoveDown(1);
            this->term.ClearChars(range.column);
        }
        this->term.MoveUp(range.line);
    }

    SlokedGraphicsPoint::Coordinate TerminalTextPane::GetMaxWidth() {
        return this->term.GetWidth();
    }

    TextPosition::Line TerminalTextPane::GetHeight() {
        return this->term.GetHeight();
    }

    const SlokedFontProperties &TerminalTextPane::GetFontProperties() const {
        return *this->fontProperties;
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