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

#ifndef SLOKED_SCREEN_TERMINAL_MULTIPLEXER_BUFFEREDGRAPHICS_H_
#define SLOKED_SCREEN_TERMINAL_MULTIPLEXER_BUFFEREDGRAPHICS_H_

#include <bitset>
#include <limits>
#include <optional>
#include <set>

#include "sloked/screen/Graphics.h"

namespace sloked {

    class BufferedGraphicsMode {
     public:
        void SetGraphicsMode(SlokedTextGraphics);
        void SetGraphicsMode(SlokedBackgroundGraphics);
        void SetGraphicsMode(SlokedForegroundGraphics);
        void apply(SlokedTerminal &) const;
        bool operator==(const BufferedGraphicsMode &) const;
        bool operator!=(const BufferedGraphicsMode &) const;

     private:
        static constexpr auto TextSize =
            static_cast<int>(SlokedTextGraphics::Concealed) + 1;
        static_assert(TextSize < 32);
        uint_fast32_t text{0};
        uint_fast32_t background = None;
        uint_fast32_t foreground = None;

        static constexpr std::size_t None =
            std::numeric_limits<uint32_t>::max();
    };
}  // namespace sloked

#endif