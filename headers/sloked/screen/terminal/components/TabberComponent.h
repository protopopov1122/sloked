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

#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_TABBERCOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_TABBERCOMPONENT_H_

#include "sloked/core/CharWidth.h"
#include "sloked/core/Encoding.h"
#include "sloked/screen/components/TabberComponent.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/multiplexer/TerminalTabber.h"
#include "sloked/screen/terminal/components/ComponentHandle.h"
#include <vector>
#include <memory>

namespace sloked {

    class TerminalTabberComponent : public SlokedTabberComponent {
     public:
        TerminalTabberComponent(SlokedTerminal &, const Encoding &, const SlokedCharWidth &);

        std::size_t GetWindowCount() const override;
        std::shared_ptr<Window> GetFocus() const override;
        std::shared_ptr<Window> GetWindow(Window::Id) const override;

        std::shared_ptr<Window> NewWindow() override;
        std::shared_ptr<Window> NewWindow(Window::Id) override;

        void Render() override;
        void UpdateDimensions() override;

     protected:
        void ProcessComponentInput(const SlokedKeyboardInput &) override;
        
     private:
        class TerminalTabberWindow : public Window {
         public:
            TerminalTabberWindow(Id, std::unique_ptr<TerminalComponentHandle>, TerminalTabberComponent &);
            bool IsOpen() const override;
            bool HasFocus() const override;
            SlokedComponentHandle &GetComponent() const override;
            Id GetId() const override;

            void SetFocus() override;
            void Move(Id) override;
            void Close() override;

            void Render();
            void Update();
            void ProcessInput(const SlokedKeyboardInput &);

         private:
            Id id;
            std::unique_ptr<TerminalComponentHandle> component;
            TerminalTabberComponent &root;
        };

        friend class TerminalTabberWindow;

        TerminalTabber tabber;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        std::vector<std::shared_ptr<TerminalTabberWindow>> components;
    };
}

#endif