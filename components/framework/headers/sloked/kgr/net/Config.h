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

#ifndef SLOKED_KGR_NET_CONFIG_H_
#define SLOKED_KGR_NET_CONFIG_H_

#include "sloked/Base.h"
#include <chrono>

namespace sloked {

    struct KgrNetConfig {
        static constexpr unsigned int Factor = 1;
        static constexpr std::chrono::system_clock::duration RequestTimeout = std::chrono::milliseconds(50) * Factor;
        static constexpr std::chrono::system_clock::duration ResponseTimeout = std::chrono::milliseconds(400) * Factor;
        static constexpr std::chrono::system_clock::duration InactivityTimeout = std::chrono::seconds(5) * Factor;
        static constexpr std::chrono::system_clock::duration InactivityThreshold = InactivityTimeout * 2 * Factor;
    };
}

#endif