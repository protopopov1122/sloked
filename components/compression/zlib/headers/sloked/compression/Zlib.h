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

#ifndef SLOKED_COMPRESSION_ZLIB_H_
#define SLOKED_COMPRESSION_ZLIB_H_

#include "sloked/core/Compression.h"

namespace sloked {

    class SlokedZlibCompression : public SlokedCompression {
     public:
        class ZlibCompressor : public Compressor {
         public:
            ZlibCompressor(Level);
            ~ZlibCompressor();
            Data Compress(SlokedSpan<const uint8_t>) final;
            Data Decompress(SlokedSpan<const uint8_t>, std::size_t) final;

         private:
            struct State;
            std::unique_ptr<State> state;
        };

        std::unique_ptr<Compressor> NewCompressor(Level = DefaultLevel) final;
    };
}  // namespace sloked

#endif