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

#ifndef SLOKED_SCREEN_COMPONENTS_TABBERCOMPONENT_H_
#define SLOKED_SCREEN_COMPONENTS_TABBERCOMPONENT_H_

#include "sloked/core/Indexed.h"
#include "sloked/screen/Component.h"
#include "sloked/screen/components/ComponentHandle.h"
#include <optional>

namespace sloked {

    class SlokedTabberComponent : public SlokedScreenComponent {
     public:
        class Window : public SlokedComponentWindow {
         public:
            virtual void Move(Id) = 0;
        };

        virtual std::size_t GetWindowCount() const = 0;
        virtual std::shared_ptr<Window> GetFocus() const = 0;
        virtual std::shared_ptr<Window> GetWindow(Window::Id) const = 0;

        virtual std::shared_ptr<Window> NewWindow() = 0;
        virtual std::shared_ptr<Window> NewWindow(Window::Id) = 0;
    };
}

#endif