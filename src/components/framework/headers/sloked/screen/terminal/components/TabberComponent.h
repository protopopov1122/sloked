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

#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_TABBERCOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_TABBERCOMPONENT_H_

#include <memory>
#include <vector>

#include "sloked/core/CharPreset.h"
#include "sloked/core/Encoding.h"
#include "sloked/screen/components/TabberComponent.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/components/ComponentHandle.h"
#include "sloked/screen/terminal/multiplexer/TerminalTabber.h"

namespace sloked {

    class TerminalTabberComponent : public SlokedTabberComponent {
     public:
        TerminalTabberComponent(SlokedTerminal &, const Encoding &,
                                const SlokedCharPreset &);

        std::size_t GetWindowCount() const final;
        std::shared_ptr<Window> GetFocus() const final;
        std::shared_ptr<Window> GetWindow(Window::Id) const final;

        std::shared_ptr<Window> NewWindow() final;
        std::shared_ptr<Window> NewWindow(Window::Id) final;

        TaskResult<void> RenderSurface() final;
        void ShowSurface() final;
        void UpdateDimensions() final;
        TextPosition GetDimensions() final;
        void OnUpdate(std::function<void()>) final;

     protected:
        void ProcessComponentInput(const SlokedKeyboardInput &) final;

     private:
        class TerminalTabberWindow : public Window {
         public:
            TerminalTabberWindow(Id, std::unique_ptr<TerminalComponentHandle>,
                                 TerminalTabberComponent &);
            bool IsOpened() const final;
            bool HasFocus() const final;
            SlokedComponentHandle &GetComponent() const final;
            Id GetId() const final;
            void SetId(Id);

            void SetFocus() final;
            void Move(Id) final;
            void Close() final;

            TaskResult<void> RenderSurface();
            void ShowSurface();
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
        const SlokedCharPreset &charPreset;
        std::vector<std::shared_ptr<TerminalTabberWindow>> components;
        std::function<void()> updateListener;
    };
}  // namespace sloked

#endif