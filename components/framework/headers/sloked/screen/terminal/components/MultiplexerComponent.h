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

#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_MULTIPLEXERCOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_MULTIPLEXERCOMPONENT_H_

#include <list>
#include <map>
#include <memory>

#include "sloked/screen/components/MultiplexerComponent.h"
#include "sloked/screen/terminal/components/ComponentHandle.h"
#include "sloked/screen/terminal/multiplexer/TerminalWindow.h"

namespace sloked {

    class TerminalMultiplexerComponent : public SlokedMultiplexerComponent {
     public:
        TerminalMultiplexerComponent(SlokedTerminal &, const Encoding &,
                                     const SlokedCharPreset &);
        virtual ~TerminalMultiplexerComponent();

        std::shared_ptr<Window> GetFocus() const final;
        std::shared_ptr<Window> GetWindow(Window::Id) const final;
        std::size_t GetWindowCount() const final;

        std::shared_ptr<Window> NewWindow(const TextPosition &,
                                          const TextPosition &) final;

        TaskResult<void> RenderSurface() final;
        void ShowSurface() final;
        void UpdateDimensions() final;
        TextPosition GetDimensions() final;
        void OnUpdate(std::function<void()>) final;

     protected:
        void ProcessComponentInput(const SlokedKeyboardInput &) final;

     private:
        class TerminalMultiplexerWindow : public Window {
         public:
            TerminalMultiplexerWindow(Id,
                                      std::unique_ptr<TerminalComponentHandle>,
                                      std::unique_ptr<TerminalWindow>,
                                      TerminalMultiplexerComponent &);
            virtual ~TerminalMultiplexerWindow();

            bool IsOpened() const final;
            bool HasFocus() const final;
            SlokedComponentHandle &GetComponent() const final;
            Id GetId() const final;

            void SetFocus() final;
            void Move(const TextPosition &) final;
            void Resize(const TextPosition &) final;
            void Close() final;

            TaskResult<void> RenderSurface();
            void ShowSurface();
            void Update();
            void ProcessInput(const SlokedKeyboardInput &);

         private:
            Id id;
            std::unique_ptr<TerminalComponentHandle> component;
            std::unique_ptr<TerminalWindow> window;
            TerminalMultiplexerComponent &root;
        };

        friend class TerminalMultiplexerWindow;

        SlokedTerminal &terminal;
        const Encoding &encoding;
        const SlokedCharPreset &charPreset;
        std::map<Window::Id, std::shared_ptr<TerminalMultiplexerWindow>>
            windows;
        std::list<Window::Id> focus;
        Window::Id nextId;
        std::function<void()> updateListener;
    };
}  // namespace sloked

#endif