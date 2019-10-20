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

#include "sloked/screen/terminal/components/TextPaneComponent.h"
#include "sloked/screen/terminal/components/TextPane.h"

namespace sloked {

    TerminalTextPaneComponent::TerminalTextPaneComponent(SlokedTerminal &term, std::unique_ptr<SlokedTextPaneWidget> widget)
        : SlokedScreenComponent(Type::TextPane), term(term), widget(std::move(widget)) {}

    void TerminalTextPaneComponent::Render() {
        if (this->widget) {
            TerminalTextPane screen(this->term);
            this->widget->Render(screen);
        }
    }

    void TerminalTextPaneComponent::UpdateDimensions() {}
    
    void TerminalTextPaneComponent::ProcessComponentInput(const SlokedKeyboardInput &input) {
        if (this->widget) {
            this->widget->ProcessInput(input);
        }
    }
}