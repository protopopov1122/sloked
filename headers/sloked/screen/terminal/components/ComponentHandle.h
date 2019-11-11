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

#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_COMPONENTHANDLE_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_COMPONENTHANDLE_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/CharWidth.h"
#include "sloked/screen/components/ComponentHandle.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/components/TextPaneComponent.h"
#include <memory>

namespace sloked {

    class TerminalComponentHandle : public SlokedComponentHandle {
     public:
        TerminalComponentHandle(SlokedTerminal &, const Encoding &, const SlokedCharWidth &);

        void Render() override;
        void UpdateDimensions() override;
        TextPosition GetDimensions() override;
        void OnUpdate(std::function<void()>) override;
        
        bool HasComponent() const override;
        SlokedScreenComponent &GetComponent() const override;
        SlokedScreenComponent &NewTextPane(std::unique_ptr<SlokedTextPaneWidget>) override;
        SlokedSplitterComponent &NewSplitter(Splitter::Direction) override;
        SlokedTabberComponent &NewTabber() override;
        SlokedMultiplexerComponent &NewMultiplexer() override;
        void Close() override;

     protected:
        void ProcessComponentInput(const SlokedKeyboardInput &) override;

     private:
        SlokedTerminal &terminal;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        std::unique_ptr<SlokedScreenComponent> component;
        std::function<void()> updateListener;
    };
}

#endif