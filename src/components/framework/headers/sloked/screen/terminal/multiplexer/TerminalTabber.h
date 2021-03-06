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

#ifndef SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALTABBER_H_
#define SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALTABBER_H_

#include <memory>
#include <optional>
#include <vector>

#include "sloked/core/Indexed.h"
#include "sloked/screen/terminal/Terminal.h"

namespace sloked {

    class TerminalTabber {
     public:
        using TabId = std::size_t;

        TerminalTabber(SlokedTerminal &);

        TabId GetTabCount() const;
        std::optional<TabId> GetCurrentTab() const;
        SlokedTerminal &GetTab(TabId) const;

        TextPosition GetDimensions();
        SlokedIndexed<SlokedTerminal &, TabId> NewTab();
        SlokedIndexed<SlokedTerminal &, TabId> NewTab(TabId);
        bool SelectTab(TabId);
        bool MoveTab(TabId, TabId);
        bool CloseTab(TabId);

     private:
        SlokedTerminal &term;
        std::vector<std::shared_ptr<SlokedTerminal>> tabs;
        TabId current_tab;
    };
}  // namespace sloked

#endif