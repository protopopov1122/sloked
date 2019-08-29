/*
  SPDX-License-Identifier: LGPL-3.0-or-later

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_SCREEN_KEYBOARD_H_
#define SLOKED_SCREEN_KEYBOARD_H_

#include "sloked/Base.h"
#include <variant>
#include <string>

namespace sloked {

    enum class SlokedControlKey {
        ArrowUp,
        ArrowDown,
        ArrowLeft,
        ArrowRight,
        Backspace,
        Delete,
        Insert,
        Escape,
        PageUp,
        PageDown,
        Home,
        End,
        Enter,
        Tab,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9
    };

    using SlokedKeyboardInput = std::variant<std::string, SlokedControlKey>;
}

#endif