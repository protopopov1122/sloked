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

#include "sloked/core/Error.h"
#include "sloked/screen/terminal/components/SplitterComponent.h"

namespace sloked {

    TerminalSplitterComponent::TerminalSplitterWindow::TerminalSplitterWindow(Window::Id id, std::unique_ptr<TerminalComponentHandle> component, TerminalSplitterComponent &root)
        : id(id), component(std::move(component)), root(root) {}

    bool TerminalSplitterComponent::TerminalSplitterWindow::IsOpen() const {
        return this->component != nullptr;
    }

    bool TerminalSplitterComponent::TerminalSplitterWindow::HasFocus() const {
        return this->root.focus == this->id;
    }

    SlokedComponentHandle &TerminalSplitterComponent::TerminalSplitterWindow::GetComponent() const {
        if (this->component) {
            return *this->component;
        } else {
            throw SlokedError("Window already closed");
        }
    }

    TerminalSplitterComponent::TerminalSplitterWindow::Id TerminalSplitterComponent::TerminalSplitterWindow::GetId() const {
        return this->id;
    }

    void TerminalSplitterComponent::TerminalSplitterWindow::SetFocus() {
        if (this->component) {
            this->root.focus = this->id;
        } else {
            throw SlokedError("Window already closed");
        }
    }
    
    void TerminalSplitterComponent::TerminalSplitterWindow::UpdateConstraints(const Splitter::Constraints &constraints) {
        if (this->component) {
            this->root.splitter.UpdateConstraints(this->id, constraints);
        } else {
            throw SlokedError("Window already closed");
        }
    }

    void TerminalSplitterComponent::TerminalSplitterWindow::Move(Id newId) {
        if (this->component) {
            if (!this->root.splitter.Move(this->id, newId)) {
                throw SlokedError("Invalid window index");
            }
            if (this->id < newId) {
                this->root.components.insert(this->root.components.begin() + newId + 1, this->root.components.at(this->id));
                this->root.components.erase(this->root.components.begin() + this->id);
            } else if (this->id > newId) {
                this->root.components.insert(this->root.components.begin() + newId, this->root.components.at(this->id));
                this->root.components.erase(this->root.components.begin() + this->id + 1);
            }

            for (std::size_t i = 0; i < this->root.components.size(); i++) {
                this->root.components.at(i)->id = i;
            }
        } else {
            throw SlokedError("Window already closed");
        }
    }

    void TerminalSplitterComponent::TerminalSplitterWindow::Close() {
        if (this->component) {
            this->root.components.erase(this->root.components.begin() + this->id);
            this->root.splitter.CloseTerminal(this->id);
            this->component = nullptr;
        } else {
            throw SlokedError("Window already closed");
        }
    }

    void TerminalSplitterComponent::TerminalSplitterWindow::Update() {
        if (this->component) {
            this->component->UpdateDimensions();
        }
    }

    void TerminalSplitterComponent::TerminalSplitterWindow::Render() {
        if (this->component) {
            this->component->Render();
        }
    }

    void TerminalSplitterComponent::TerminalSplitterWindow::ProcessInput(const SlokedKeyboardInput &input) {
        if (this->component) {
            this->component->ProcessInput(input);
        }
    }

    TerminalSplitterComponent::TerminalSplitterComponent(SlokedTerminal &term, Splitter::Direction dir, const Encoding &enc, const SlokedCharWidth &chWidth)
        : splitter(term, dir, enc, chWidth), encoding(enc), charWidth(chWidth), focus(0) {}

    std::shared_ptr<TerminalSplitterComponent::Window> TerminalSplitterComponent::GetFocus() const {
        if (this->focus < this->components.size()) {
            return this->components.at(this->focus);
        } else {
            return nullptr;
        }
    }

    std::shared_ptr<TerminalSplitterComponent::Window> TerminalSplitterComponent::GetWindow(Window::Id id) const {
        if (id < this->components.size()) {
            return this->components.at(id);
        } else {
            return nullptr;
        }
    }

    std::size_t TerminalSplitterComponent::GetWindowCount() const {
        return this->components.size();
    }

    std::shared_ptr<TerminalSplitterComponent::Window> TerminalSplitterComponent::NewWindow(const Splitter::Constraints &constraints) {
        auto term = this->splitter.NewTerminal(constraints);
        auto component = std::make_unique<TerminalComponentHandle>(term.value, this->encoding, this->charWidth);
        auto window = std::make_shared<TerminalSplitterWindow>(term.index, std::move(component), *this);
        this->components.push_back(window);
        return window;
    }

    std::shared_ptr<TerminalSplitterComponent::Window> TerminalSplitterComponent::NewWindow(Window::Id idx, const Splitter::Constraints &constraints) {
        auto term = this->splitter.NewTerminal(idx, constraints);
        auto component = std::make_unique<TerminalComponentHandle>(term.value, this->encoding, this->charWidth);
        auto window = std::make_shared<TerminalSplitterWindow>(term.index, std::move(component), *this);
        this->components.insert(this->components.begin() + idx, window);
        return window;
    }

    void TerminalSplitterComponent::Render() {
        for (std::size_t i = 0; i < this->components.size(); i++) {
            if (i != this->focus) {
                this->components.at(i)->Render();
            }
        }
        if (this->focus < this->components.size()) {
            this->components.at(this->focus)->Render();
        }
    }

    void TerminalSplitterComponent::UpdateDimensions() {
        this->splitter.Update();
        for (auto &win : this->components) {
            win->Update();
        }
    }

    void TerminalSplitterComponent::ProcessComponentInput(const SlokedKeyboardInput &input) {
        if (this->focus < this->components.size()) {
            this->components.at(this->focus)->ProcessInput(input);
        }
    }
}