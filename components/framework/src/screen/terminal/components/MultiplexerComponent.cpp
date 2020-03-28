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

#include "sloked/screen/terminal/components/MultiplexerComponent.h"

#include "sloked/core/Error.h"

namespace sloked {

    TerminalMultiplexerComponent::TerminalMultiplexerWindow::
        TerminalMultiplexerWindow(
            Id id, std::unique_ptr<TerminalComponentHandle> component,
            std::unique_ptr<TerminalWindow> window,
            TerminalMultiplexerComponent &root)
        : id(id), component(std::move(component)), window(std::move(window)),
          root(root) {}

    TerminalMultiplexerComponent::TerminalMultiplexerWindow::
        ~TerminalMultiplexerWindow() {
        if (this->IsOpened()) {
            this->Close();
        }
    }

    bool TerminalMultiplexerComponent::TerminalMultiplexerWindow::IsOpened()
        const {
        return this->root.windows.count(this->id) != 0;
    }

    bool TerminalMultiplexerComponent::TerminalMultiplexerWindow::HasFocus()
        const {
        return this->root.focus.back() == this->id;
    }

    SlokedComponentHandle &
        TerminalMultiplexerComponent::TerminalMultiplexerWindow::GetComponent()
            const {
        if (this->component) {
            return *this->component;
        } else {
            throw SlokedError("Window already closed");
        }
    }

    TerminalMultiplexerComponent::TerminalMultiplexerWindow::Window::Id
        TerminalMultiplexerComponent::TerminalMultiplexerWindow::GetId() const {
        return this->id;
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::SetFocus() {
        if (this->IsOpened()) {
            this->root.focus.erase(
                std::remove(this->root.focus.begin(), this->root.focus.end(),
                            this->id),
                this->root.focus.end());
            this->root.focus.push_back(this->id);
            if (this->root.updateListener) {
                this->root.updateListener();
            }
        } else {
            throw SlokedError("Window already closed");
        }
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::Move(
        const TextPosition &pos) {
        if (this->window) {
            this->window->Move(pos);
            if (this->root.updateListener) {
                this->root.updateListener();
            }
        } else {
            throw SlokedError("Window already closed");
        }
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::Resize(
        const TextPosition &pos) {
        if (this->window) {
            this->window->Resize(pos);
            if (this->root.updateListener) {
                this->root.updateListener();
            }
        } else {
            throw SlokedError("Window already closed");
        }
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::Close() {
        if (this->IsOpened()) {
            this->root.focus.erase(
                std::remove(this->root.focus.begin(), this->root.focus.end(),
                            this->id),
                this->root.focus.end());
            this->component.reset();
            this->window.reset();
            this->root.windows.erase(this->id);
            if (this->root.updateListener) {
                this->root.updateListener();
            }
        } else {
            throw SlokedError("Window already closed");
        }
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::Render() {
        if (this->component) {
            this->component->Render();
        }
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::Update() {
        if (this->component) {
            this->window->UpdateDimensions();
            this->component->UpdateDimensions();
        }
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::ProcessInput(
        const SlokedKeyboardInput &input) {
        if (this->component) {
            this->component->ProcessInput(input);
        }
    }

    TerminalMultiplexerComponent::TerminalMultiplexerComponent(
        SlokedTerminal &term, const Encoding &encoding,
        const SlokedCharPreset &charPreset)
        : SlokedMultiplexerComponent(Type::Multiplexer), terminal(term),
          encoding(encoding), charPreset(charPreset), focus(0), nextId(0) {}

    TerminalMultiplexerComponent::~TerminalMultiplexerComponent() {
        for (auto it = this->windows.begin(); it != this->windows.end();) {
            auto current = it->second;
            ++it;
            current->Close();
        }
        this->windows.clear();
    }

    std::shared_ptr<TerminalMultiplexerComponent::Window>
        TerminalMultiplexerComponent::GetFocus() const {
        if (!this->focus.empty()) {
            return this->windows.at(this->focus.back());
        } else {
            return nullptr;
        }
    }

    std::shared_ptr<TerminalMultiplexerComponent::Window>
        TerminalMultiplexerComponent::GetWindow(Window::Id id) const {
        if (this->windows.count(id)) {
            return this->windows.at(id);
        } else {
            return nullptr;
        }
    }

    std::size_t TerminalMultiplexerComponent::GetWindowCount() const {
        return this->windows.size();
    }

    std::shared_ptr<TerminalMultiplexerComponent::Window>
        TerminalMultiplexerComponent::NewWindow(const TextPosition &pos,
                                                const TextPosition &dim) {
        auto terminal = std::make_unique<TerminalWindow>(
            this->terminal, this->encoding, this->charPreset, pos, dim);
        auto handle = std::make_unique<TerminalComponentHandle>(
            *terminal, this->encoding, this->charPreset);
        auto id = this->nextId++;
        auto window = std::make_shared<TerminalMultiplexerWindow>(
            id, std::move(handle), std::move(terminal), *this);
        window->GetComponent().OnUpdate(this->updateListener);
        this->windows[id] = window;
        this->focus.push_back(id);
        if (this->updateListener) {
            this->updateListener();
        }
        return window;
    }

    void TerminalMultiplexerComponent::Render() {
        for (auto id : this->focus) {
            this->windows.at(id)->Render();
        }
    }

    void TerminalMultiplexerComponent::UpdateDimensions() {
        for (auto kv : this->windows) {
            kv.second->Update();
        }
    }

    TextPosition TerminalMultiplexerComponent::GetDimensions() {
        return {this->terminal.GetHeight(), this->terminal.GetWidth()};
    }

    void TerminalMultiplexerComponent::OnUpdate(
        std::function<void()> listener) {
        this->updateListener = listener;
        for (auto &win : this->windows) {
            win.second->GetComponent().OnUpdate(listener);
        }
    }

    void TerminalMultiplexerComponent::ProcessComponentInput(
        const SlokedKeyboardInput &input) {
        if (!this->focus.empty()) {
            this->windows.at(this->focus.back())->ProcessInput(input);
        }
    }
}  // namespace sloked