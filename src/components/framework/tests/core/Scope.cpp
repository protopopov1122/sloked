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

#include "sloked/core/Scope.h"

#include "catch2/catch.hpp"

using namespace sloked;

TEST_CASE("'At exit' does nothing for nullptr lambda") {
    auto atExit = std::make_unique<AtScopeExit>(nullptr);
    SECTION("Manual detach") {
        REQUIRE_NOTHROW(atExit->Detach());
        REQUIRE_NOTHROW(atExit.reset());
    }
    SECTION("Destructor") {
        REQUIRE_NOTHROW(atExit.reset());
    }
}

TEST_CASE("'At exit' invokes provided function only once") {
    bool result{false};
    auto atExit = std::make_unique<AtScopeExit>([&result] {
        REQUIRE_FALSE(result);
        result = true;
    });
    SECTION("Manual detach") {
        REQUIRE_FALSE(result);
        REQUIRE_NOTHROW(atExit->Detach());
        REQUIRE(result);
        REQUIRE_NOTHROW(atExit->Detach());
        REQUIRE_NOTHROW(atExit.reset());
        REQUIRE(result);
    }
    SECTION("Destructor") {
        REQUIRE_FALSE(result);
        REQUIRE_NOTHROW(atExit.reset());
        REQUIRE(result);
    }
}

TEST_CASE("'At exit' corectly implements move constructor") {
    bool result{false};
    AtScopeExit atExit([&result] {
        REQUIRE_FALSE(result);
        result = true;
    });
    AtScopeExit atExit2(std::move(atExit));
    REQUIRE_FALSE(result);
    REQUIRE_NOTHROW(atExit.Detach());
    REQUIRE_FALSE(result);
    REQUIRE_NOTHROW(atExit2.Detach());
    REQUIRE(result);
}

TEST_CASE("'At exit' corectly implements move assignment") {
    bool result{false}, result2{false};
    AtScopeExit atExit([&result] {
        REQUIRE_FALSE(result);
        result = true;
    });
    AtScopeExit atExit2([&result2] {
        REQUIRE_FALSE(result2);
        result2 = true;
    });
    REQUIRE_FALSE(result2);
    atExit2 = std::move(atExit);
    REQUIRE(result2);
    REQUIRE_FALSE(result);
    REQUIRE_NOTHROW(atExit.Detach());
    REQUIRE_FALSE(result);
    REQUIRE_NOTHROW(atExit2.Detach());
    REQUIRE(result);
}

TEST_CASE("'At exit' corectly implements self-move assignment") {
    bool result{false};
    AtScopeExit atExit([&result] {
        REQUIRE_FALSE(result);
        result = true;
    });
    REQUIRE_FALSE(result);
    atExit = std::move(atExit);
    REQUIRE_FALSE(result);
    REQUIRE_NOTHROW(atExit.Detach());
    REQUIRE(result);
}