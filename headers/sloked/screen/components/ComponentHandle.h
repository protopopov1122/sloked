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

#ifndef SLOKED_SCREEN_COMPONENTS_COMPONENTHANDLE_H_
#define SLOKED_SCREEN_COMPONENTS_COMPONENTHANDLE_H_

#include "sloked/screen/Component.h"
#include "sloked/screen/Splitter.h"
#include "sloked/screen/widgets/TextPaneWidget.h"
#include <memory>

namespace sloked {

    class SlokedSplitterComponent;
    class SlokedTabberComponent;
    class SlokedMultiplexerComponent;

    class SlokedComponentHandle : public SlokedScreenComponent {
     public:
        virtual SlokedScreenComponent &NewTextPane(std::unique_ptr<SlokedTextPaneWidget>) = 0;
        virtual SlokedSplitterComponent &NewSplitter(Splitter::Direction) = 0;
        virtual SlokedTabberComponent &NewTabber() = 0;
        virtual SlokedMultiplexerComponent &NewMultiplexer() = 0;
    };

    class SlokedComponentWindow {
     public:
        using Id = std::size_t;
        virtual ~SlokedComponentWindow() = default;
        virtual bool IsOpen() const = 0;
        virtual bool HasFocus() const = 0;
        virtual SlokedComponentHandle &GetComponent() const = 0;
        virtual Id GetId() const = 0;

        virtual void SetFocus() = 0;
        virtual void Close() = 0;
    };
}

#endif