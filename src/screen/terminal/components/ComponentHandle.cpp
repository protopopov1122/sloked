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

#include "sloked/screen/terminal/components/ComponentHandle.h"
#include "sloked/screen/terminal/components/SplitterComponent.h"
#include "sloked/screen/terminal/components/TabberComponent.h"
#include "sloked/screen/terminal/components/MultiplexerComponent.h"
#include "sloked/core/Error.h"

namespace sloked {

    TerminalComponentHandle::TerminalComponentHandle(SlokedTerminal &term, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : SlokedComponentHandle(Type::Handle), terminal(term), encoding(encoding), charWidth(charWidth), component(nullptr) {}

    bool TerminalComponentHandle::HasComponent() const {
        return this->component != nullptr;
    }

    SlokedScreenComponent &TerminalComponentHandle::GetComponent() const {
        if (this->component) {
            return *this->component;
        } else {
            throw SlokedError("ComponentHandle: Component not defined");
        }
    }

    SlokedScreenComponent &TerminalComponentHandle::NewTextPane(std::unique_ptr<SlokedTextPaneWidget> widget) {
        auto component = std::make_unique<TerminalTextPaneComponent>(this->terminal, std::move(widget));
        TerminalTextPaneComponent &ref = *component;
        this->component = std::move(component);
        return ref;
    }

    SlokedSplitterComponent &TerminalComponentHandle::NewSplitter(Splitter::Direction dir) {
        auto component = std::make_unique<TerminalSplitterComponent>(this->terminal, dir, this->encoding, this->charWidth);
        TerminalSplitterComponent &ref = *component;
        this->component = std::move(component);
        return ref;
    }

    SlokedTabberComponent &TerminalComponentHandle::NewTabber() {
        auto component = std::make_unique<TerminalTabberComponent>(this->terminal, this->encoding, this->charWidth);
        TerminalTabberComponent &ref = *component;
        this->component = std::move(component);
        return ref;
    }

    SlokedMultiplexerComponent &TerminalComponentHandle::NewMultiplexer() {
        auto component = std::make_unique<TerminalMultiplexerComponent>(this->terminal, this->encoding, this->charWidth);
        TerminalMultiplexerComponent &ref = *component;
        this->component = std::move(component);
        return ref;
    }

    void TerminalComponentHandle::Close() {
        this->component.reset();
    }

    void TerminalComponentHandle::Render() {
        if (this->component) {
            this->component->Render();
        }
    }

    void TerminalComponentHandle::UpdateDimensions() {
        this->terminal.Update();
        if (this->component) {
            this->component->UpdateDimensions();
        }
    }

    TextPosition TerminalComponentHandle::GetDimensions() {
        return { this->terminal.GetHeight(), this->terminal.GetWidth() };
    }

    void TerminalComponentHandle::ProcessComponentInput(const SlokedKeyboardInput &input) {
        if (this->component) {
            this->component->ProcessInput(input);
        }
    }
}