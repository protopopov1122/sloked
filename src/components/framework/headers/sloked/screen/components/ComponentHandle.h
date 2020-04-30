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

#ifndef SLOKED_SCREEN_COMPONENTS_COMPONENTHANDLE_H_
#define SLOKED_SCREEN_COMPONENTS_COMPONENTHANDLE_H_

#include <memory>

#include "sloked/screen/Component.h"
#include "sloked/screen/Splitter.h"
#include "sloked/screen/widgets/TextPaneWidget.h"

namespace sloked {

    class SlokedSplitterComponent;
    class SlokedTabberComponent;
    class SlokedMultiplexerComponent;
    class SlokedTextPaneComponent;

    class SlokedComponentHandle : public SlokedScreenComponent {
     public:
        virtual bool HasComponent() const = 0;
        virtual SlokedScreenComponent &GetComponent() const = 0;
        virtual SlokedTextPaneComponent &NewTextPane(
            std::unique_ptr<SlokedTextPaneWidget>) = 0;
        virtual SlokedSplitterComponent &NewSplitter(Splitter::Direction) = 0;
        virtual SlokedTabberComponent &NewTabber() = 0;
        virtual SlokedMultiplexerComponent &NewMultiplexer() = 0;
        virtual void Close() = 0;

     protected:
        using SlokedScreenComponent::SlokedScreenComponent;
    };

    class SlokedComponentWindow {
     public:
        using Id = std::size_t;
        virtual ~SlokedComponentWindow() = default;
        virtual bool IsOpened() const = 0;
        virtual bool HasFocus() const = 0;
        virtual SlokedComponentHandle &GetComponent() const = 0;
        virtual Id GetId() const = 0;

        virtual void SetFocus() = 0;
        virtual void Close() = 0;
    };
}  // namespace sloked

#endif