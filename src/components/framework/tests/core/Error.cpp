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

#include "sloked/core/Error.h"

#include "catch2/catch.hpp"

using namespace sloked;

TEST_CASE("Error stores error message") {
    const std::string Message = GENERATE("hello", "world", "ERROR MSG");
    try {
        throw SlokedError(Message);
    } catch (const SlokedError &ex) {
        REQUIRE(std::string{ex.what()} == Message);
    }
}