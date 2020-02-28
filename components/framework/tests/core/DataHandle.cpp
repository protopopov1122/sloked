/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "catch2/catch.hpp"
#include "sloked/core/DataHandle.h"

using namespace sloked;

struct Dummy {
    Dummy(bool &flag) : flag(flag) {
        this->flag = true;
    }

    ~Dummy() {
        this->flag = false;
    }

    bool &flag;
};

TEST_CASE("Data handle does not release content until it's being destructed") {
    bool flag;
    auto handle = SlokedTypedDataHandle<Dummy>::Wrap(std::make_unique<Dummy>(flag));
    REQUIRE(flag);
    auto handle2 = std::move(handle);;
    REQUIRE(flag);
    handle.reset();
    REQUIRE(flag);
    handle2.reset();
    REQUIRE_FALSE(flag);
}