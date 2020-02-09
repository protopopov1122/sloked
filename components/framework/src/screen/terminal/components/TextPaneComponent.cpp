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

#include "sloked/screen/terminal/components/TextPaneComponent.h"
#include "sloked/screen/terminal/components/TextPane.h"

namespace sloked {

    TerminalTextPaneComponent::TerminalTextPaneComponent(SlokedTerminal &term, std::unique_ptr<SlokedTextPaneWidget> widget)
        : SlokedTextPaneComponent(Type::TextPane), term(term), widget(std::move(widget)), screen(term) {}

    void TerminalTextPaneComponent::Render() {
        if (this->widget) {
            this->widget->Render(this->screen);
        }
    }

    void TerminalTextPaneComponent::UpdateDimensions() {
        this->term.UpdateDimensions();
    }

    TextPosition TerminalTextPaneComponent::GetDimensions() {
        return { this->term.GetHeight(), this->term.GetWidth() };
    }

    void TerminalTextPaneComponent::OnUpdate(std::function<void()> listener) {
        this->widget->OnUpdate(std::move(listener));
    }
    
    const SlokedFontProperties &TerminalTextPaneComponent::GetFontProperties() const {
        return this->screen.GetFontProperties();
    }
    
    void TerminalTextPaneComponent::ProcessComponentInput(const SlokedKeyboardInput &input) {
        if (this->widget) {
            this->widget->ProcessInput(input);
        }
    }
}