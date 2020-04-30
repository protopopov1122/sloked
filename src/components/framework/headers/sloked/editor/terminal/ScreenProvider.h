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

#ifndef SLOKED_EDITOR_POSIX_SCREENPROVIDER_H_
#define SLOKED_EDITOR_POSIX_SCREENPROVIDER_H_

#include "sloked/editor/ScreenServer.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/TerminalSize.h"
#include "sloked/screen/terminal/components/ComponentHandle.h"

namespace sloked {

    class SlokedTerminalScreenProvider : public SlokedScreenProvider {
     public:
        SlokedTerminalScreenProvider(SlokedTerminal &, const Encoding &,
                                     const SlokedCharPreset &,
                                     SlokedTerminalInputSource &,
                                     SlokedScreenSize &);
        void Render(std::function<void(SlokedScreenComponent &)>) final;
        std::vector<SlokedKeyboardInput> ReceiveInput(
            std::chrono::system_clock::duration) final;
        SlokedMonitor<SlokedScreenComponent &> &GetScreen() final;
        SlokedScreenSize &GetSize() final;
        const Encoding &GetEncoding() final;

     private:
        SlokedTerminal &terminal;
        const Encoding &encoding;
        TerminalComponentHandle rootComponent;
        SlokedMonitor<SlokedScreenComponent &> screen;
        SlokedTerminalInputSource &inputSource;
        SlokedScreenSize &screenSize;
    };
}  // namespace sloked

#endif