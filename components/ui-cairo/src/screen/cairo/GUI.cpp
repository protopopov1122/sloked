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

#include "sloked/screen/cairo/GUI.h"
#include "sloked/screen/cairo/SDL.h"
#include "sloked/screen/terminal/CairoTerminal.h"
#include <atomic>

namespace sloked {

    static std::atomic_bool PangoInitialized{false};

    SlokedCairoSDLGraphicalComponents::SlokedCairoSDLGraphicalComponents(SlokedScreenManager &screenManager)
        : screenManager(screenManager) {
        if (!PangoInitialized.exchange(true)) {
            Pango::init();
        }
    }

    std::unique_ptr<SlokedGraphicalTerminalWindow> SlokedCairoSDLGraphicalComponents::OpenTerminal(const SlokedGraphicalTerminalWindow::Parameters &prms) {
        auto sdlWindow = std::make_unique<SlokedSDLCairoWindow>(this->screenManager, prms.size, prms.title);
        auto terminal = std::make_unique<SlokedCairoTerminal>(prms.font, prms.defaultMode);
        auto window = std::make_unique<SlokedCairoTerminalWindow>(std::move(sdlWindow), std::move(terminal));
        return window;
    }
}