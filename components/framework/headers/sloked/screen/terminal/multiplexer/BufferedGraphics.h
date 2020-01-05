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

#ifndef SLOKED_SCREEN_TERMINAL_MULTIPLEXER_BUFFEREDGRAPHICS_H_
#define SLOKED_SCREEN_TERMINAL_MULTIPLEXER_BUFFEREDGRAPHICS_H_

#include "sloked/screen/Graphics.h"
#include <set>
#include <optional>
#include <bitset>
#include <limits>

namespace sloked {

    class BufferedGraphicsMode {
     public:
        void SetGraphicsMode(SlokedTextGraphics);
        void SetGraphicsMode(SlokedBackgroundGraphics);
        void SetGraphicsMode(SlokedForegroundGraphics);
        void apply(SlokedTerminal &) const;
        bool operator==(const BufferedGraphicsMode &) const;

     private:
        std::bitset<static_cast<int>(SlokedTextGraphics::Count)> text;
        uint32_t background = None;
        uint32_t foreground = None;

        static constexpr std::size_t None = std::numeric_limits<uint32_t>::max();
    };
}

#endif