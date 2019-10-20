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

#include "sloked/screen/Component.h"
#include "sloked/screen/components/ComponentHandle.h"
#include "sloked/screen/components/MultiplexerComponent.h"
#include "sloked/screen/components/SplitterComponent.h"
#include "sloked/screen/components/TabberComponent.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedScreenComponent::SlokedScreenComponent(Type type)
        : type(type) {}

    SlokedScreenComponent::Type SlokedScreenComponent::GetType() const {
        return this->type;
    }

    SlokedComponentHandle &SlokedScreenComponent::AsHandle() {
        if (this->type == Type::Handle) {
            return *static_cast<SlokedComponentHandle *>(this);
        } else {
            throw SlokedError("Component: Not a handle");
        }
    }

    SlokedMultiplexerComponent &SlokedScreenComponent::AsMultiplexer() {
        if (this->type == Type::Multiplexer) {
            return *static_cast<SlokedMultiplexerComponent *>(this);
        } else {
            throw SlokedError("Component: Not a multiplexer");
        }
    }

    SlokedSplitterComponent &SlokedScreenComponent::AsSplitter() {
        if (this->type == Type::Splitter) {
            return *static_cast<SlokedSplitterComponent *>(this);
        } else {
            throw SlokedError("Component: Not a splitter");
        }
    }

    SlokedTabberComponent &SlokedScreenComponent::AsTabber() {
        if (this->type == Type::Tabber) {
            return *static_cast<SlokedTabberComponent *>(this);
        } else {
            throw SlokedError("Component: Not a tabber");
        }
    }
    
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