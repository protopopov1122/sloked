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

#include "sloked/editor/terminal/ScreenProvider.h"

namespace sloked {

    SlokedTerminalScreenProvider::SlokedTerminalScreenProvider(SlokedTerminal &terminal, const Encoding &encoding, const SlokedCharPreset &charPreset,
        SlokedTerminalInputSource &inputSource, SlokedScreenSize &screenSize)
        : terminal(terminal), encoding(encoding), rootComponent(terminal, encoding, charPreset),
            screen(rootComponent), inputSource(inputSource), screenSize(screenSize) {}

    void SlokedTerminalScreenProvider::Render(std::function<void(SlokedScreenComponent &)> render) {
        this->screen.Lock([&](auto &screen) {
            this->terminal.SetGraphicsMode(SlokedTextGraphics::Off);
            this->terminal.ClearScreen();
            render(screen);
            this->terminal.RenderFrame();
        });
    }

    std::vector<SlokedKeyboardInput> SlokedTerminalScreenProvider::ReceiveInput(std::chrono::system_clock::duration timeout) {
        if (this->inputSource.WaitInput(timeout)) {
            return this->inputSource.GetInput();
        } else {
            return {};
        }
    }

    SlokedMonitor<SlokedScreenComponent &> &SlokedTerminalScreenProvider::GetScreen() {
        return this->screen;
    }

    SlokedScreenSize &SlokedTerminalScreenProvider::GetSize() {
        return this->screenSize;
    }

    const Encoding &SlokedTerminalScreenProvider::GetEncoding() {
        return this->encoding;
    }
}