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

#ifndef SLOKED_CORE_VLQ_H_
#define SLOKED_CORE_VLQ_H_

#include <cinttypes>
#include <limits>
#include <optional>
#include <type_traits>

#include "sloked/Base.h"

namespace sloked {

    class SlokedVLQ {
     public:
        template <typename N, typename T>
        static void Encode(N number, T out) {
            static_assert(std::is_unsigned_v<N>,
                          "VLQ accepts only unsigned integers");
            constexpr uint8_t Mask = 0b10000000;
            do {
                uint8_t byte = number & (~Mask);
                number >>= 7;
                if (number == 0) {
                    byte |= Mask;
                }
                *out = byte;
            } while (number != 0);
        }

        template <typename N, typename I>
        static std::optional<std::pair<N, I>> Decode(I begin, I end) {
            static_assert(std::is_unsigned_v<N>,
                          "VLQ accepts only unsigned integers");
            constexpr uint8_t Mask = 0b10000000;
            N result{0};
            bool finished = false;
            std::size_t offset = 0;
            while (!finished && begin != end) {
                const uint8_t byte = *begin++;
                result |= (byte & (~Mask)) << offset;
                offset += 7;
                finished = (byte & Mask) != 0;
            }
            if (finished) {
                return std::make_pair(result, begin);
            } else {
                return {};
            }
        }
    };
}  // namespace sloked

#endif