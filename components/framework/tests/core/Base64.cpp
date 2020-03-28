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

#include "sloked/core/Base64.h"

#include "catch2/catch.hpp"

using namespace sloked;

std::string B64Encode(std::string_view input) {
    return SlokedBase64::Encode(input.begin(), input.end());
}

std::string B64Decode(std::string_view input) {
    auto res = SlokedBase64::Decode(input);
    return {res.begin(), res.end()};
}

TEST_CASE("Base64 is correctly encoding strings") {
    REQUIRE(B64Encode("") == "");
    REQUIRE(B64Encode("f") == "Zg==");
    REQUIRE(B64Encode("fo") == "Zm8=");
    REQUIRE(B64Encode("foo") == "Zm9v");
    REQUIRE(B64Encode("foob") == "Zm9vYg==");
    REQUIRE(B64Encode("fooba") == "Zm9vYmE=");
    REQUIRE(B64Encode("foobar") == "Zm9vYmFy");
    REQUIRE(B64Encode("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123"
                      "456789+/") ==
            "QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVphYmNkZWZnaGlqa2xtbm9wcXJzdHV2d3"
            "h5ejAxMjM0NTY3ODkrLw==");
}

TEST_CASE("Base64 is correctly decoding strings") {
    REQUIRE(B64Decode("") == "");
    REQUIRE(B64Decode("Zg==") == "f");
    REQUIRE(B64Decode("Zm8=") == "fo");
    REQUIRE(B64Decode("Zm9v") == "foo");
    REQUIRE(B64Decode("Zm9vYg==") == "foob");
    REQUIRE(B64Decode("Zm9vYmE=") == "fooba");
    REQUIRE(B64Decode("Zm9vYmFy") == "foobar");
    REQUIRE(B64Decode("QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVphYmNkZWZnaGlqa2xtbm9w"
                      "cXJzdHV2d3h5ejAxMjM0NTY3ODkrLw==") ==
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
}