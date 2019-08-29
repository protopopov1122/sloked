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

#include "sloked/screen/Component.h"

namespace sloked {

    SlokedScreenComponent::SlokedScreenComponent() = default;
    
    void SlokedScreenComponent::ProcessInput(const SlokedKeyboardInput &input) {
        bool res = false;
        if (this->inputHandler) {
            res = this->inputHandler(input);
        }
        if (!res) {
            this->ProcessComponentInput(input);
        }
    }

    void SlokedScreenComponent::SetInputHandler(InputHandler handler) {
        this->inputHandler = handler;
    }

    void SlokedScreenComponent::ResetInputHandler() {
        this->inputHandler = 0;
    }
}