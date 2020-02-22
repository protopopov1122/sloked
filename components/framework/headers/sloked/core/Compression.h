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

#ifndef SLOKED_CORE_COMPRESSION_H_
#define SLOKED_CORE_COMPRESSION_H_

#include "sloked/core/Span.h"
#include <vector>
#include <cinttypes>
#include <memory>

namespace sloked {

    class SlokedCompression {
     public:
        class Compressor {
         public:
            using Unit = uint8_t;
            using Data = std::vector<uint8_t>;
            virtual ~Compressor() = default;
            virtual Data Compress(SlokedSpan<const uint8_t>) = 0;
            virtual Data Decompress(SlokedSpan<const uint8_t>, std::size_t) = 0;
        };

        using Level = int8_t;
        static constexpr Level MinLevel{1};
        static constexpr Level MaxLevel{10};
        static constexpr Level DefaultLevel{0};

        virtual ~SlokedCompression() = default;
        virtual std::unique_ptr<Compressor> NewCompressor(Level = DefaultLevel) = 0;
    };
}

#endif