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

#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_COMPONENTHANDLE_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_COMPONENTHANDLE_H_

#include <memory>

#include "sloked/core/CharPreset.h"
#include "sloked/core/Encoding.h"
#include "sloked/screen/components/ComponentHandle.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/components/TextPaneComponent.h"

namespace sloked {

    class TerminalComponentHandle : public SlokedComponentHandle {
     public:
        TerminalComponentHandle(SlokedTerminal &, const Encoding &,
                                const SlokedCharPreset &);

        TaskResult<void> RenderSurface() final;
        void ShowSurface() final;
        void UpdateDimensions() final;
        TextPosition GetDimensions() final;
        void OnUpdate(std::function<void()>) final;

        bool HasComponent() const final;
        SlokedScreenComponent &GetComponent() const final;
        SlokedTextPaneComponent &NewTextPane(
            std::unique_ptr<SlokedTextPaneWidget>) final;
        SlokedSplitterComponent &NewSplitter(Splitter::Direction) final;
        SlokedTabberComponent &NewTabber() final;
        SlokedMultiplexerComponent &NewMultiplexer() final;
        void Close() final;

     protected:
        void ProcessComponentInput(const SlokedKeyboardInput &) final;

     private:
        SlokedTerminal &terminal;
        const Encoding &encoding;
        const SlokedCharPreset &charPreset;
        std::unique_ptr<SlokedScreenComponent> component;
        std::function<void()> updateListener;
    };
}  // namespace sloked

#endif