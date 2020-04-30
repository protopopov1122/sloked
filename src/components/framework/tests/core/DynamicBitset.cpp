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

#include "sloked/core/DynamicBitset.h"

#include "catch2/catch.hpp"

using namespace sloked;

TEST_CASE("Dynamic bitset correctly stores bits") {
    SlokedDynamicBitset bset;
    REQUIRE_NOTHROW(bset.Set(0, true));
    REQUIRE_NOTHROW(bset.Set(10, false));
    REQUIRE_NOTHROW(bset.Set(100, true));
    REQUIRE_NOTHROW(bset.Set(1000, false));
    REQUIRE(bset.Get(0));
    REQUIRE_FALSE(bset.Get(10));
    REQUIRE(bset.Get(100));
    REQUIRE_FALSE(bset.Get(1000));
}

TEST_CASE("Dynamic bitset grows gradually") {
    SlokedDynamicBitset bset;
    REQUIRE(bset.Count() == 0);
    bset.Set(0, true);
    bset.Set(SlokedDynamicBitset::MinWidth - 1, true);
    REQUIRE(bset.Count() == SlokedDynamicBitset::MinWidth);
    bset.Set(SlokedDynamicBitset::MinWidth, true);
    bset.Set(SlokedDynamicBitset::MinWidth + 1, true);
    REQUIRE(bset.Count() == SlokedDynamicBitset::MinWidth * 2);
    bset.Set(SlokedDynamicBitset::MinWidth * 2, true);
    bset.Set(SlokedDynamicBitset::MinWidth * 2 + 1, true);
    REQUIRE(bset.Count() == SlokedDynamicBitset::MinWidth * 3);
}

TEST_CASE("Dynamic bitset allocates unique values") {
    SlokedDynamicBitset bset;
    for (std::size_t i = 0; i < SlokedDynamicBitset::MinWidth * 2; i++) {
        REQUIRE(bset.Allocate() == i);
    }
    bset.Set(5, false);
    bset.Set(6, false);
    bset.Set(7, false);
    REQUIRE(bset.Allocate() == 5);
    REQUIRE(bset.Allocate() == 6);
    REQUIRE(bset.Allocate() == 7);
}