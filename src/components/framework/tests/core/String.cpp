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

#include "sloked/core/String.h"

#include "catch2/catch.hpp"

using namespace sloked;

TEST_CASE("starts_with correctly detects prefixes") {
    REQUIRE(starts_with("Hello, world!", ""));
    REQUIRE(starts_with("Hello, world!", "H"));
    REQUIRE(starts_with("Hello, world!", "Hello"));
    REQUIRE_FALSE(starts_with("Hello, world!", "hello"));
    REQUIRE(starts_with("Hello, world!", "Hello, world"));
    REQUIRE(starts_with("Hello, world!", "Hello, world!"));
    REQUIRE_FALSE(starts_with("Hello, world!", "Hello, world!!!"));
    REQUIRE_FALSE(starts_with("Hello, world!", "ABCDEF"));
    REQUIRE_FALSE(starts_with("Hello, world!", "\n"));
    REQUIRE(starts_with("\n\n\t", "\n"));
}

TEST_CASE("ends_with correctly detects suffixes") {
    REQUIRE(ends_with("Hello, world!", ""));
    REQUIRE(ends_with("Hello, world!", "\0"));
    REQUIRE(ends_with("Hello, world!", "!"));
    REQUIRE(ends_with("Hello, world!", "world!"));
    REQUIRE(ends_with("Hello, world!", " world!"));
    REQUIRE_FALSE(ends_with("Hello, world!", "World!"));
    REQUIRE(ends_with("Hello, world!", "ello, world!"));
    REQUIRE(ends_with("Hello, world!", "Hello, world!"));
    REQUIRE_FALSE(ends_with("Hello, world!", "HHHello, world!"));
    REQUIRE_FALSE(ends_with("Hello, world!", "Hello, world!!"));
    REQUIRE_FALSE(ends_with("Hello, world!", "ABCDEF"));
    REQUIRE_FALSE(ends_with("Hello, world!", "\t"));
    REQUIRE(ends_with("\n\n\t", "\t"));
}