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

#ifndef SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALSPLITTER_H_
#define SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALSPLITTER_H_

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "sloked/core/CharPreset.h"
#include "sloked/core/Encoding.h"
#include "sloked/core/Indexed.h"
#include "sloked/screen/Splitter.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/multiplexer/TerminalWindow.h"

namespace sloked {

    class TerminalSplitter {
     public:
        using WinId = std::size_t;

        TerminalSplitter(SlokedTerminal &, Splitter::Direction,
                         const Encoding &, const SlokedCharPreset &);

        SlokedTerminal &GetTerminal(WinId) const;
        WinId GetTerminalCount() const;
        Splitter::Direction GetDirection() const;
        const Splitter::Constraints &GetConstraints(WinId) const;

        SlokedIndexed<SlokedTerminal &, WinId> NewTerminal(
            const Splitter::Constraints &);
        SlokedIndexed<SlokedTerminal &, WinId> NewTerminal(
            WinId, const Splitter::Constraints &);
        bool UpdateConstraints(WinId, const Splitter::Constraints &);
        bool Move(WinId, WinId);
        bool CloseTerminal(WinId);
        void Update();
        TextPosition GetDimensions();

     private:
        using Window =
            std::pair<std::shared_ptr<TerminalWindow>, Splitter::Constraints>;
        unsigned int GetMinimum() const;

        SlokedTerminal &term;
        Splitter::Direction direction;
        const Encoding &encoding;
        const SlokedCharPreset &charPreset;
        std::vector<Window> windows;
    };
}  // namespace sloked

#endif