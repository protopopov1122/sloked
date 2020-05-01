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

#include "sloked/core/Hash.h"

#include "catch2/catch.hpp"

using namespace sloked;

SlokedCrc32::Checksum StringCrc32(const std::string &str) {
    return SlokedCrc32::Calculate(str.begin(), str.end());
}

TEST_CASE("CRC32 is calculated correctly") {
    REQUIRE(StringCrc32("Hello, world!") == 0xEBE6C6E6);
    REQUIRE(StringCrc32(" ") == 0xE96CCF45);
    REQUIRE(StringCrc32("1234567890") == 0x261DAEE5);
    REQUIRE(StringCrc32("abcABCxyzXYZ") == 0xB8A1A7A7);
    REQUIRE(StringCrc32("YOBA ETO TI?") == 0x32B2C6D0);
}