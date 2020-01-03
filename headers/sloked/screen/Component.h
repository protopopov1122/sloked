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

#ifndef SLOKED_SCREEN_COMPONENT_H_
#define SLOKED_SCREEN_COMPONENT_H_

#include "sloked/core/Position.h"
#include "sloked/screen/Keyboard.h"
#include <functional>
#include <map>

namespace sloked {
    
    class SlokedComponentHandle;
    class SlokedMultiplexerComponent;
    class SlokedSplitterComponent;
    class SlokedTabberComponent;
    class SlokedScreenComponent;

    class SlokedComponentListener {
     private:
        friend class SlokedScreenComponent;
        SlokedComponentListener(SlokedScreenComponent &component, std::size_t id)
            : component(std::addressof(component)), id(id) {}

        SlokedScreenComponent *component;
        std::size_t id;
    };

    class SlokedScreenComponent {
     public:
        using InputHandler = std::function<bool(const SlokedKeyboardInput &)>;
        enum class Type {
           Handle,
           Multiplexer,
           Splitter,
           Tabber,
           TextPane
        };

        virtual ~SlokedScreenComponent() = default;

        Type GetType() const;
        SlokedComponentHandle &AsHandle();
        SlokedMultiplexerComponent &AsMultiplexer();
        SlokedSplitterComponent &AsSplitter();
        SlokedTabberComponent &AsTabber();

        void ProcessInput(const SlokedKeyboardInput &);
        SlokedComponentListener AttachInputHandler(InputHandler);
        void DetachInputHandle(const SlokedComponentListener &);
        
        virtual void Render() = 0;
        virtual void UpdateDimensions() = 0;
        virtual TextPosition GetDimensions() = 0;
        virtual void OnUpdate(std::function<void()>) = 0;

     protected:
        SlokedScreenComponent(Type);
        virtual void ProcessComponentInput(const SlokedKeyboardInput &) = 0;

     private:
        Type type;
        std::map<std::size_t, InputHandler> inputHandler;
        std::size_t nextInputId;
    };
}

#endif