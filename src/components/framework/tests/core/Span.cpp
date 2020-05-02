
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

#include "sloked/core/Span.h"

#include "catch2/catch.hpp"

using namespace sloked;

TEST_CASE("Span correctly handles null pointers") {
    SlokedSpan<const char> empty;
    SlokedSpan<const char> empty2(nullptr, 0);
    REQUIRE_THROWS(SlokedSpan<const char>(nullptr, 100));

    REQUIRE(empty.Empty());
    REQUIRE(empty.Data() == nullptr);
    REQUIRE(empty.Size() == 0);
    REQUIRE_THROWS(empty.Front());
    REQUIRE_THROWS(empty.Back());
    REQUIRE_THROWS(empty[0]);
    REQUIRE_THROWS(empty.Subspan(0, 1));
    REQUIRE(empty2.Empty());
    REQUIRE(empty2.Data() == nullptr);
    REQUIRE(empty2.Size() == 0);
    REQUIRE_THROWS(empty2.Front());
    REQUIRE_THROWS(empty2.Back());
    REQUIRE_THROWS(empty2[0]);
    REQUIRE_THROWS(empty2.Subspan(0, 1));
}

TEST_CASE("Span correctly stores reference to memory") {
    char Data[] = "Hello, world!";
    const std::size_t Length = sizeof(Data) / sizeof(char);

    SlokedSpan<char> origin(Data, Length);
    SlokedSpan<const char> span = origin;
    REQUIRE(origin.Data() == span.Data());
    REQUIRE(origin.Size() == span.Size());
    REQUIRE_FALSE(span.Empty());
    REQUIRE(span.Data() == Data);
    REQUIRE(span.Size() == Length);

    for (std::size_t i = 0; i < span.Size(); i++) {
        REQUIRE(span[i] == Data[i]);
    }
    REQUIRE(span.Front() == Data[0]);
    REQUIRE(span.Back() == Data[Length]);

    constexpr std::size_t Offset = 7;
    auto subspan = span.Subspan(Offset, span.Size() - Offset);
    REQUIRE_FALSE(subspan.Empty());
    REQUIRE(subspan.Data() == Data + Offset);
    REQUIRE(subspan.Size() == Length - Offset);
}

TEST_CASE("Span move semantics works correctly") {
    const char Data[] = "Hello, world!";
    const std::size_t Length = sizeof(Data) / sizeof(char);

    SlokedSpan<const char> span1(Data, Length);
    SlokedSpan<const char> span2(std::move(span1));
    REQUIRE(span1.Empty());
    REQUIRE(span1.Data() == nullptr);
    REQUIRE(span1.Size() == 0);
    REQUIRE_FALSE(span2.Empty());
    REQUIRE(span2.Data() == Data);
    REQUIRE(span2.Size() == Length);

    auto span3 = std::move(span2);
    REQUIRE(span2.Empty());
    REQUIRE(span2.Data() == nullptr);
    REQUIRE(span2.Size() == 0);
    REQUIRE_FALSE(span3.Empty());
    REQUIRE(span3.Data() == Data);
    REQUIRE(span3.Size() == Length);
}