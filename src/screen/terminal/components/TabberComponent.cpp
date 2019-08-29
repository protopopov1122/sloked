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
#include "sloked/screen/terminal/components/TabberComponent.h"

namespace sloked {

    TerminalTabberComponent::TerminalTabberWindow::TerminalTabberWindow(Id id, std::unique_ptr<TerminalComponentHandle> component, TerminalTabberComponent &root)
        : id(id), component(std::move(component)), root(root) {}
    
    bool TerminalTabberComponent::TerminalTabberWindow::IsOpen() const {
        return this->component != nullptr;
    }

    bool TerminalTabberComponent::TerminalTabberWindow::HasFocus() const {
        return this->id == this->root.tabber.GetCurrentTab();
    }

    SlokedComponentHandle &TerminalTabberComponent::TerminalTabberWindow::GetComponent() const {
        if (this->component) {
            return *this->component;
        } else {
            throw SlokedError("Window already closed");
        }
    }

    TerminalTabberComponent::TerminalTabberWindow::Id TerminalTabberComponent::TerminalTabberWindow::GetId() const {
        return this->id;
    }

    void TerminalTabberComponent::TerminalTabberWindow::SetFocus() {
        if (this->component) {
            this->root.tabber.SelectTab(this->id);
        } else {
            throw SlokedError("Window already closed");
        }
    }

    void TerminalTabberComponent::TerminalTabberWindow::Move(Id newId) {
        if (this->component) {
            if (!this->root.tabber.MoveTab(this->id, newId)) {
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

    void TerminalTabberComponent::TerminalTabberWindow::Close() {
        if (this->component) {
            this->root.components.erase(this->root.components.begin() + this->id);
            this->root.tabber.CloseTab(this->id);
            this->component = nullptr;
        } else {
            throw SlokedError("Window already closed");
        }
    }

    void TerminalTabberComponent::TerminalTabberWindow::Render() {
        if (this->component) {
            this->component->Render();
        }
    }

    void TerminalTabberComponent::TerminalTabberWindow::Update() {
        if (this->component) {
            this->component->UpdateDimensions();
        }
    }

    void TerminalTabberComponent::TerminalTabberWindow::ProcessInput(const SlokedKeyboardInput &input) {
        if (this->component) {
            this->component->ProcessInput(input);
        }
    }

    TerminalTabberComponent::TerminalTabberComponent(SlokedTerminal &term, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : tabber(term), encoding(encoding), charWidth(charWidth) {}

    std::size_t TerminalTabberComponent::GetWindowCount() const {
        return this->components.size();
    }

    std::shared_ptr<TerminalTabberComponent::Window> TerminalTabberComponent::GetFocus() const {
        auto tabId = this->tabber.GetCurrentTab();
        if (tabId.has_value()) {
            return this->components.at(tabId.value());
        } else {
            return nullptr;
        }
    }

    std::shared_ptr<TerminalTabberComponent::Window> TerminalTabberComponent::GetWindow(Window::Id idx) const {
        if (idx < this->components.size()) {
            return this->components.at(idx);
        } else {
            return nullptr;
        }
    }

    std::shared_ptr<TerminalTabberComponent::Window> TerminalTabberComponent::NewWindow() {
        auto term = this->tabber.NewTab();
        auto component = std::make_unique<TerminalComponentHandle>(term.value, this->encoding, this->charWidth);
        auto window = std::make_shared<TerminalTabberWindow>(term.index, std::move(component), *this);
        this->components.push_back(window);
        return window;
    }

    std::shared_ptr<TerminalTabberComponent::Window> TerminalTabberComponent::NewWindow(Window::Id idx) {
        auto term = this->tabber.NewTab(idx);
        auto component = std::make_unique<TerminalComponentHandle>(term.value, this->encoding, this->charWidth);
        auto window = std::make_shared<TerminalTabberWindow>(term.index, std::move(component), *this);
        this->components.push_back(window);
        return window;
    }

    void TerminalTabberComponent::Render() {
        auto idx = this->tabber.GetCurrentTab();
        if (idx.has_value() && idx < this->components.size()) {
            this->components.at(idx.value())->Render();
        }
    }

    void TerminalTabberComponent::UpdateDimensions() {
        for (auto &tab : this->components) {
            tab->Update();
        }
    }

    void TerminalTabberComponent::ProcessComponentInput(const SlokedKeyboardInput &input) {
        auto idx = this->tabber.GetCurrentTab();
        if (idx.has_value() && idx < this->components.size()) {
            this->components.at(idx.value())->ProcessInput(input);
        }
    }
}