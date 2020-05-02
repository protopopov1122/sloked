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

#include "sloked/core/RangeMap.h"

#include "catch2/catch.hpp"

using namespace sloked;

TEST_CASE("Range map correctly inserts and retrieves ranges") {
    RangeMap<int, int> map(0);

    REQUIRE_FALSE(map.Has(-100));
    REQUIRE_FALSE(map.Has(-1));
    REQUIRE_FALSE(map.Has(0));
    REQUIRE_FALSE(map.Has(1));
    REQUIRE_FALSE(map.Has(100));

    REQUIRE_THROWS(map.At(-100));
    REQUIRE_THROWS(map.At(-1));
    REQUIRE_THROWS(map.At(0));
    REQUIRE_THROWS(map.At(1));
    REQUIRE_THROWS(map.At(100));

    REQUIRE_THROWS(map.Insert(-100, 100, 0));
    REQUIRE_THROWS(map.Insert(-1, 100, 0));
    REQUIRE_THROWS(map.Insert(20, 10, 0));
    REQUIRE_THROWS(map.Insert(20, 20, 0));

    REQUIRE_NOTHROW(map.Insert(0, 100, 0));
    REQUIRE_NOTHROW(map.Insert(10, 90, 1));
    REQUIRE_NOTHROW(map.Insert(10, 20, 2));
    REQUIRE_NOTHROW(map.Insert(50, 95, 3));

    for (int i = 0; i < 10; i++) {
        REQUIRE(map.At(i) == 0);
    }
    for (int i = 10; i < 20; i++) {
        REQUIRE(map.At(i) == 2);
    }
    for (int i = 20; i < 50; i++) {
        REQUIRE(map.At(i) == 1);
    }
    for (int i = 50; i < 95; i++) {
        REQUIRE(map.At(i) == 3);
    }
    for (int i = 95; i < 100; i++) {
        REQUIRE(map.At(i) == 0);
    }
    REQUIRE_THROWS(map.At(100));
    REQUIRE_THROWS(map.At(200));
    REQUIRE_THROWS(map.At(1000));
}