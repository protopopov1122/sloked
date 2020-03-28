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

#include "sloked/compression/Zlib.h"

#include <zlib.h>

#include "sloked/core/Error.h"

namespace sloked {

    struct SlokedZlibCompression::ZlibCompressor::State {
        z_stream compress;
        z_stream decompress;
    };

    SlokedZlibCompression::ZlibCompressor::ZlibCompressor(Level level)
        : state(std::make_unique<State>()) {
        this->state->compress.zalloc = Z_NULL;
        this->state->compress.zfree = Z_NULL;
        this->state->compress.opaque = Z_NULL;

        this->state->compress.avail_in = 0;
        this->state->compress.next_in = Z_NULL;
        this->state->compress.avail_out = 0;
        this->state->compress.next_out = Z_NULL;
        switch (level) {
            case DefaultLevel:
                deflateInit(&this->state->compress, Z_DEFAULT_COMPRESSION);
                break;

            case MinLevel:
                deflateInit(&this->state->compress, Z_NO_COMPRESSION);
                break;

            default:
                static_assert(MaxLevel - MinLevel >= 3);
                if (level < (MaxLevel - (MinLevel + 1)) / 2) {
                    deflateInit(&this->state->compress, Z_BEST_SPEED);
                } else {
                    deflateInit(&this->state->compress, Z_BEST_COMPRESSION);
                }
                break;
        }

        this->state->decompress.zalloc = Z_NULL;
        this->state->decompress.zfree = Z_NULL;
        this->state->decompress.opaque = Z_NULL;

        this->state->decompress.avail_in = 0;
        this->state->decompress.next_in = Z_NULL;
        this->state->decompress.avail_out = 0;
        this->state->decompress.next_out = Z_NULL;
        inflateInit(&this->state->decompress);
    }

    SlokedZlibCompression::ZlibCompressor::~ZlibCompressor() {
        deflateEnd(&this->state->compress);
        inflateEnd(&this->state->decompress);
    }

    SlokedZlibCompression::Compressor::Data
        SlokedZlibCompression::ZlibCompressor::Compress(
            SlokedSpan<const uint8_t> input) {
        Data output;
        output.insert(output.end(),
                      deflateBound(&this->state->compress, input.Size()), 0);

        this->state->compress.avail_in = input.Size();
        this->state->compress.next_in = (Bytef *) input.Data();
        this->state->compress.avail_out = (uInt) output.size();
        this->state->compress.next_out = (Bytef *) output.data();

        auto err = deflate(&this->state->compress, Z_FINISH);
        if (err == Z_STREAM_END) {
            output.erase(output.begin() + this->state->compress.total_out,
                         output.end());
            deflateReset(&this->state->compress);
            return output;
        } else {
            deflateReset(&this->state->compress);
            throw SlokedError("ZlibCompressor: Compression error");
        }
    }

    SlokedZlibCompression::Compressor::Data
        SlokedZlibCompression::ZlibCompressor::Decompress(
            SlokedSpan<const uint8_t> input, std::size_t size) {
        Data output;
        output.insert(output.end(), size, 0);

        this->state->decompress.avail_in = (uInt) input.Size();
        this->state->decompress.next_in = (Bytef *) input.Data();
        this->state->decompress.avail_out = (uInt) size;
        this->state->decompress.next_out = (Bytef *) output.data();

        auto err = inflate(&this->state->decompress, Z_NO_FLUSH);
        inflateReset(&this->state->decompress);
        if (err == Z_STREAM_END) {
            return output;
        } else {
            throw SlokedError("ZlibCompressor: Compression error");
        }
    }

    std::unique_ptr<SlokedZlibCompression::Compressor>
        SlokedZlibCompression::NewCompressor(Level level) {
        return std::make_unique<ZlibCompressor>(level);
    }
}  // namespace sloked