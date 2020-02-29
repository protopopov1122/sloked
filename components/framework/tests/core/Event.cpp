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
#include "sloked/core/Event.h"

using namespace sloked;

TEST_CASE("Typed event emitter notifies listener about new events") {
    SlokedEventEmitter<int> emitter;
    emitter.Emit(1);
    std::vector<int> history;
    SlokedEventEmitter<int>::Listener callback = [&history](int value) {
        history.push_back(value);
    };
    auto unbind1 = emitter.Listen(callback);
    emitter.Emit(2);
    auto unbind2 = emitter.Listen(callback);
    emitter.Emit(3);
    auto unbind3 = emitter.Listen(callback);
    emitter.Emit(4);
    unbind1();
    emitter.Emit(5);
    unbind2();
    emitter.Emit(6);
    unbind3();
    emitter.Emit(7);
    REQUIRE(history == std::vector<int>{2, 3, 3, 4, 4, 4, 5, 5, 6});
    REQUIRE_NOTHROW(unbind1());
    REQUIRE_NOTHROW(unbind2());
    REQUIRE_NOTHROW(unbind3());
}

TEST_CASE("Untyped event emitter notifies listener about new events") {
    SlokedEventEmitter<void> emitter;
    std::size_t counter{0};
    SlokedEventEmitter<void>::Listener callback = [&counter]() {
        counter++;
    };
    emitter.Emit();
    auto unbind1 = emitter.Listen(callback);
    REQUIRE(counter == 0);
    emitter.Emit();
    REQUIRE(counter == 1);
    auto unbind2 = emitter.Listen(callback);
    emitter.Emit();
    REQUIRE(counter == 3);
    auto unbind3 = emitter.Listen(callback);
    emitter.Emit();
    REQUIRE(counter == 6);
    unbind1();
    emitter.Emit();
    REQUIRE(counter == 8);
    unbind2();
    emitter.Emit();
    REQUIRE(counter == 9);
    unbind3();
    emitter.Emit();
    REQUIRE(counter == 9);
    REQUIRE_NOTHROW(unbind1());
    REQUIRE_NOTHROW(unbind2());
    REQUIRE_NOTHROW(unbind3());
}