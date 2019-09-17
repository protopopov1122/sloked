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

#ifndef SLOKED_SCREEN_COMPONENT_H_
#define SLOKED_SCREEN_COMPONENT_H_

#include "sloked/core/Position.h"
#include "sloked/screen/Keyboard.h"
#include <functional>

namespace sloked {

    class SlokedScreenComponent {
     public:
        using InputHandler = std::function<bool(const SlokedKeyboardInput &)>;

        SlokedScreenComponent();
        virtual ~SlokedScreenComponent() = default;

        void ProcessInput(const SlokedKeyboardInput &);
        void SetInputHandler(InputHandler);
        void ResetInputHandler();
        
        virtual void Render() = 0;
        virtual void UpdateDimensions() = 0;

     protected:
        virtual void ProcessComponentInput(const SlokedKeyboardInput &) = 0;

     private:
        InputHandler inputHandler;
    };
}

#endif