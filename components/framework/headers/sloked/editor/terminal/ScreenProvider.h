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

#ifndef SLOKED_EDITOR_POSIX_SCREENPROVIDER_H_
#define SLOKED_EDITOR_POSIX_SCREENPROVIDER_H_

#include "sloked/editor/ScreenServer.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/TerminalSize.h"
#include "sloked/screen/terminal/components/ComponentHandle.h"

namespace sloked {

    template <typename ResizeListener>
    class SlokedTerminalScreenProvider : public SlokedScreenProvider {
     public:
        SlokedTerminalScreenProvider(SlokedTerminal &terminal, const Encoding &encoding, const SlokedCharPreset &charPreset, SlokedTerminalInputSource &inputSource)
            : terminal(terminal), encoding(encoding), rootComponent(terminal, encoding, charPreset), screen(rootComponent), inputSource(inputSource), size(terminal) {}

        void Render(std::function<void(SlokedScreenComponent &)> render) final {
            this->screen.Lock([&](auto &screen) {
                this->terminal.SetGraphicsMode(SlokedTextGraphics::Off);
                this->terminal.ClearScreen();
                render(screen);
                this->terminal.RenderFrame();
            });
        }

        std::vector<SlokedKeyboardInput> ReceiveInput(std::chrono::system_clock::duration timeout) final {
            if (this->inputSource.WaitInput(timeout)) {
                return this->inputSource.GetInput();
            } else {
                return {};
            }
        }

        SlokedMonitor<SlokedScreenComponent &> &GetScreen() final {
            return this->screen;
        }

        SlokedScreenSize &GetSize() final {
            return this->size;
        }

        const Encoding &GetEncoding() final {
            return this->encoding;
        }

     private:
        SlokedTerminal &terminal;
        const Encoding &encoding;
        TerminalComponentHandle rootComponent;
        SlokedMonitor<SlokedScreenComponent &> screen;
        SlokedTerminalInputSource &inputSource;
        SlokedTerminalSize<ResizeListener> size;
    };
}

#endif