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

#include "sloked/core/LRU.h"

#include "catch2/catch.hpp"

using namespace sloked;

TEST_CASE("LRU removes least-recently used records") {
    SlokedLRUCache<int, int> lru(99);
    for (int i = 0; i < 100; i++) {
        lru.Insert(i, i * 100);
    }
    for (int i = 0; i < 50; i++) {
        REQUIRE(lru.At(i) == i * 100);
    }
    for (int i = -50; i < 0; i++) {
        lru.Emplace(i, int{i * 100});
    }
    for (int i = -50; i < 0; i++) {
        REQUIRE(lru.At(i) == i * 100);
    }
    for (int i = -100; i < 100; i++) {
        if (i >= -50 && i < 50) {
            REQUIRE(lru.Has(i));
            REQUIRE(lru.At(i) == i * 100);
        } else {
            REQUIRE_FALSE(lru.Has(i));
            REQUIRE_THROWS(lru.At(i));
        }
    }
}