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

#ifndef SLOKED_SCREEN_COMPONENTS_SPLITTERCOMPONENT_H_
#define SLOKED_SCREEN_COMPONENTS_SPLITTERCOMPONENT_H_

#include "sloked/core/Indexed.h"
#include "sloked/screen/Component.h"
#include "sloked/screen/components/ComponentHandle.h"
#include <optional>

namespace sloked {

    class SlokedSplitterComponent : public SlokedScreenComponent {
     public:
        class Window : public SlokedComponentWindow {
         public:
            virtual void UpdateConstraints(const Splitter::Constraints &) = 0;
            virtual void Move(Id) = 0;
        };

        virtual std::shared_ptr<Window> GetFocus() const = 0;
        virtual std::shared_ptr<Window> GetWindow(Window::Id) const = 0;
        virtual std::size_t GetWindowCount() const = 0;

        virtual std::shared_ptr<Window> NewWindow(const Splitter::Constraints &) = 0;
        virtual std::shared_ptr<Window> NewWindow(Window::Id, const Splitter::Constraints &) = 0;
    };
}

#endif