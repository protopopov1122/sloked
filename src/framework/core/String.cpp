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

#include "sloked/core/String.h"

namespace sloked {

    bool starts_with(std::string_view s1, std::string_view s2) {
        if (s1.size() >= s2.size()) {
            for (std::size_t i = 0; i < s2.size(); i++) {
                if (s1[i] != s2[i]) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    }
}