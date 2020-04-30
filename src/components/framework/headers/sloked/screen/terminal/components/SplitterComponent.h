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

#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_SPLITTERCOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_SPLITTERCOMPONENT_H_

#include <memory>
#include <vector>

#include "sloked/core/CharPreset.h"
#include "sloked/core/Encoding.h"
#include "sloked/screen/components/SplitterComponent.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/components/ComponentHandle.h"
#include "sloked/screen/terminal/multiplexer/TerminalSplitter.h"

namespace sloked {

    class TerminalSplitterComponent : public SlokedSplitterComponent {
     public:
        TerminalSplitterComponent(SlokedTerminal &, Splitter::Direction,
                                  const Encoding &, const SlokedCharPreset &);
        ~TerminalSplitterComponent();

        std::shared_ptr<Window> GetFocus() const final;
        std::shared_ptr<Window> GetWindow(Window::Id) const final;
        std::size_t GetWindowCount() const final;

        std::shared_ptr<Window> NewWindow(const Splitter::Constraints &) final;
        std::shared_ptr<Window> NewWindow(Window::Id,
                                          const Splitter::Constraints &) final;

        TaskResult<void> RenderSurface() final;
        void ShowSurface() final;
        void UpdateDimensions() final;
        TextPosition GetDimensions() final;
        void OnUpdate(std::function<void()>) final;

     protected:
        void ProcessComponentInput(const SlokedKeyboardInput &) final;

     private:
        class TerminalSplitterWindow : public Window {
         public:
            TerminalSplitterWindow(Window::Id,
                                   std::unique_ptr<TerminalComponentHandle>,
                                   TerminalSplitterComponent &);
            bool IsOpened() const final;
            bool HasFocus() const final;
            SlokedComponentHandle &GetComponent() const final;
            Id GetId() const final;
            void SetId(Id);

            void SetFocus() final;
            void UpdateConstraints(const Splitter::Constraints &) final;
            void Move(Id) final;
            void Close() final;

            void Update();
            TaskResult<void> RenderSurface();
            void ShowSurface();
            void ProcessInput(const SlokedKeyboardInput &);

         private:
            Window::Id id;
            std::unique_ptr<TerminalComponentHandle> component;
            TerminalSplitterComponent &root;
        };

        friend class TerminalSplitterWindow;

        TerminalSplitter splitter;
        const Encoding &encoding;
        const SlokedCharPreset &charPreset;
        std::vector<std::shared_ptr<TerminalSplitterWindow>> components;
        Window::Id focus;
        std::function<void()> updateListener;
        std::shared_ptr<SlokedStandardLifetime> lifetime;
    };
}  // namespace sloked

#endif