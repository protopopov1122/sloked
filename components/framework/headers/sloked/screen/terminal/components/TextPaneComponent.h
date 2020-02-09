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

#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_TEXTPANECOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_TEXTPANECOMPONENT_H_

#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/widgets/TextPaneWidget.h"
#include "sloked/screen/components/TextPane.h"
#include "sloked/screen/terminal/components/TextPane.h"
#include <memory>

namespace sloked {

    class TerminalTextPaneComponent : public SlokedTextPaneComponent {
     public:
        TerminalTextPaneComponent(SlokedTerminal &, std::unique_ptr<SlokedTextPaneWidget>);
        
        void Render() override;
        void UpdateDimensions() override;
        TextPosition GetDimensions() override;
        void OnUpdate(std::function<void()>) override;
        const SlokedFontProperties &GetFontProperties() const override;

        
     protected:
        using SlokedTextPaneComponent::SlokedTextPaneComponent;
        void ProcessComponentInput(const SlokedKeyboardInput &) override;

     private:
        SlokedTerminal &term;
        std::unique_ptr<SlokedTextPaneWidget> widget;
        TerminalTextPane screen;
    };
}

#endif