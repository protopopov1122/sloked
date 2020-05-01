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

#include "sloked/core/CharPreset.h"

#include "catch2/catch.hpp"
#include "sloked/core/Locale.h"

using namespace sloked;

TEST_CASE("Char preset correctly stores tab width") {
    SlokedCharPreset charPreset;
    const Encoding &encoding = SlokedLocale::SystemEncoding();
    for (std::size_t i = 1; i < 16; i++) {
        charPreset.SetTabWidth(i);
        REQUIRE(charPreset.GetCharWidth(U'\t', 0) == i);
        REQUIRE(charPreset.GetTab(encoding, 0) == std::string(i, ' '));
    }
}

TEST_CASE("Char preset leaves other char width unchanged") {
    SlokedCharPreset charPreset;
    charPreset.SetTabWidth(4);
    for (char32_t chr = 0; chr < 255; chr++) {
        if (chr != U'\t') {
            REQUIRE(charPreset.GetCharWidth(chr, 0) == 1);
        }
    }
}

TEST_CASE("Char preset correctly calculates character positions") {
    SlokedCharPreset charPreset;
    const Encoding &encoding = SlokedLocale::SystemEncoding();
    const std::string sample{"\tA\tB\tC\tD"};
    charPreset.SetTabWidth(4);
    REQUIRE(charPreset.GetRealPosition(sample, 0, encoding) ==
            std::pair<std::size_t, std::size_t>{0, 4});
    REQUIRE(charPreset.GetRealPosition(sample, 1, encoding) ==
            std::pair<std::size_t, std::size_t>{4, 5});
    REQUIRE(charPreset.GetRealPosition(sample, 2, encoding) ==
            std::pair<std::size_t, std::size_t>{5, 8});
    REQUIRE(charPreset.GetRealPosition(sample, 3, encoding) ==
            std::pair<std::size_t, std::size_t>{8, 9});
    REQUIRE(charPreset.GetRealPosition(sample, 4, encoding) ==
            std::pair<std::size_t, std::size_t>{9, 12});
    REQUIRE(charPreset.GetRealPosition(sample, 5, encoding) ==
            std::pair<std::size_t, std::size_t>{12, 13});
    REQUIRE(charPreset.GetRealPosition(sample, 6, encoding) ==
            std::pair<std::size_t, std::size_t>{13, 16});
    REQUIRE(charPreset.GetRealPosition(sample, 7, encoding) ==
            std::pair<std::size_t, std::size_t>{16, 17});
}

TEST_CASE("Char preset notifies listeners about preset changes") {
    SlokedCharPreset charPreset;
    charPreset.SetTabWidth(4);
    std::size_t widthSum = 0;
    auto unbind = charPreset.Listen([&widthSum](const auto &charPreset) {
        widthSum += charPreset.GetCharWidth(U'\t', 0);
    });
    charPreset.SetTabWidth(1);
    charPreset.SetTabWidth(2);
    charPreset.SetTabWidth(3);
    charPreset.SetTabWidth(4);
    unbind();
    charPreset.SetTabWidth(5);
    REQUIRE(widthSum == 10);
}