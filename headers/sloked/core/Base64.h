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

#ifndef SLOKED_CORE_BASE64_H_
#define SLOKED_CORE_BASE64_H_

#include "sloked/Base.h"
#include <string>
#include <vector>
#include <cinttypes>

namespace sloked {

    class SlokedBase64 {
     public:
        using Byte = uint8_t;
        using Iterator = const Byte *;
        static std::string Encode(Iterator, const Iterator);
        static std::vector<Byte> Decode(std::string_view);

        template <typename T>
        static std::string Encode(const T *begin, const T *end) {
            return SlokedBase64::Encode(reinterpret_cast<Iterator>(begin), reinterpret_cast<Iterator>(end));
        }
    };
}

#endif