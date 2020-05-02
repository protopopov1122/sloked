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

#include "sloked/core/VLQ.h"

#include "catch2/catch.hpp"

using namespace sloked;

std::vector<uint8_t> VLQEncode(uint64_t value) {
    std::vector<uint8_t> buffer;
    SlokedVLQ::Encode(value, std::back_inserter(buffer));
    return buffer;
}

std::optional<uint64_t> VLQDecode(std::vector<uint8_t> value) {
    auto result = SlokedVLQ::Decode<uint64_t>(value.begin(), value.end());
    if (!result.has_value()) {
        return {};
    } else {
        REQUIRE(result.value().second == value.end());
        return result.value().first;
    }
}

TEST_CASE("VLQ correctly encodes numbers") {
    REQUIRE(VLQEncode(0) == std::vector<uint8_t>{0b10000000});
    REQUIRE(VLQEncode(1) == std::vector<uint8_t>{0b10000001});
    REQUIRE(VLQEncode(64) == std::vector<uint8_t>{0b11000000});
    REQUIRE(VLQEncode(127) == std::vector<uint8_t>{0b11111111});
    REQUIRE(VLQEncode(128) == std::vector<uint8_t>{0b00000000, 0b10000001});
    REQUIRE(VLQEncode(192) == std::vector<uint8_t>{0b01000000, 0b10000001});
    REQUIRE(VLQEncode(255) == std::vector<uint8_t>{0b01111111, 0b10000001});
    REQUIRE(VLQEncode(256) == std::vector<uint8_t>{0b00000000, 0b10000010});
    REQUIRE(VLQEncode(65536) ==
            std::vector<uint8_t>{0b00000000, 0b00000000, 0b10000100});
}

TEST_CASE("VLQ correctly decodes numbers") {
    REQUIRE(VLQDecode({0b10000000}) == 0);
    REQUIRE(VLQDecode({0b10000001}) == 1);
    REQUIRE(VLQDecode({0b11000000}) == 64);
    REQUIRE(VLQDecode({0b11111111}) == 127);
    REQUIRE(VLQDecode({0b00000000, 0b10000001}) == 128);
    REQUIRE(VLQDecode({0b01000000, 0b10000001}) == 192);
    REQUIRE(VLQDecode({0b01111111, 0b10000001}) == 255);
    REQUIRE(VLQDecode({0b00000000, 0b10000010}) == 256);
    REQUIRE(VLQDecode({0b00000000, 0b00000000, 0b10000100}) == 65536);
}