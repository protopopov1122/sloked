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

#ifndef SLOKED_SCREEN_GRAPHICS_TERMINAL_H_
#define SLOKED_SCREEN_GRAPHICS_TERMINAL_H_

#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/graphics/Window.h"
#include "sloked/screen/Size.h"

namespace sloked {

    class SlokedGraphicalTerminal : public SlokedDuplexTerminal {
     public:
        struct Mode {
            SlokedBackgroundGraphics background{SlokedBackgroundGraphics::White};
            SlokedForegroundGraphics foreground{SlokedForegroundGraphics::Black};
            bool bold{false};
            bool underscore{false};
        };

        virtual const std::string &GetFont() const = 0;
        virtual const Mode &GetDefaultMode() const = 0;
        virtual void SetDefaultMode(const Mode &) = 0;
        virtual SlokedScreenSize &GetTerminalSize() = 0;
    };

    class SlokedGraphicalTerminalWindow : public SlokedAbstractGraphicalWindow {
     public:
        struct Parameters {
            Parameters(SlokedGraphicsDimensions, const std::string &);
            Parameters &Size(SlokedGraphicsDimensions);
            Parameters &Title(const std::string &);
            Parameters &Font(const std::string &);
            Parameters &DefaultMode(const SlokedGraphicalTerminal::Mode &);

            SlokedGraphicsDimensions size;
            std::string title{""};
            std::string font;
            SlokedGraphicalTerminal::Mode defaultMode{};
        };

        virtual SlokedGraphicalTerminal &GetTerminal() = 0;
    };
}

#endif