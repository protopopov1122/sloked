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

#include "sloked/core/RingBuffer.h"

#include "catch2/catch.hpp"

using namespace sloked;

// TODO: Move operation and iterator tests

TEST_CASE("Static ring buffer correctly stores elements") {
    SlokedRingBuffer<int> ring(100);

    REQUIRE(ring.GetType() == SlokedRingBufferType::Static);
    REQUIRE(ring.empty());
    REQUIRE(ring.capacity() == 100);
    REQUIRE(ring.available() == ring.capacity());
    REQUIRE(ring.size() == 0);

    REQUIRE_THROWS(ring.front());
    REQUIRE_THROWS(ring.back());
    REQUIRE_THROWS(ring.pop_front());
    REQUIRE_THROWS(ring.pop_front(10));
    REQUIRE_THROWS(ring.pop_front(100));
    REQUIRE_THROWS(ring.at(0));
    REQUIRE_THROWS(ring.at(50));
    REQUIRE_THROWS(ring.at(100));

    REQUIRE_NOTHROW(ring.clear());
    REQUIRE(ring.empty());

    for (int i = 0; i < 25; i++) {
        REQUIRE_NOTHROW(ring.push_back(i));
        REQUIRE_NOTHROW(ring.emplace_back(int{i * 100}));
    }

    REQUIRE(ring.size() == 50);
    REQUIRE_FALSE(ring.empty());
    REQUIRE(ring.available() == 50);
    REQUIRE(ring.capacity() == 100);
    for (int i = 0; i < 50; i++) {
        if (i % 2 == 0) {
            REQUIRE(ring.at(i) == i / 2);
        } else {
            REQUIRE(ring.at(i) == (i / 2) * 100);
        }
        REQUIRE(ring.at(i) == ring[i]);
    }

    while (ring.available() > 0) {
        REQUIRE_NOTHROW(ring.push_back(ring.available()));
    }
    REQUIRE_THROWS(ring.push_back(0));
    REQUIRE_THROWS(ring.emplace_back(0));
    REQUIRE_FALSE(ring.empty());
    REQUIRE(ring.available() == 0);
    REQUIRE(ring.capacity() == 100);
    REQUIRE(ring.size() == 100);

    for (int i = 0; ring.available() < 50; i++) {
        if (i % 2 == 0) {
            REQUIRE(ring.front() == i / 2);
        } else {
            REQUIRE(ring.front() == (i / 2) * 100);
        }
        REQUIRE_NOTHROW(ring.pop_front());
    }
    REQUIRE(ring.size() == 50);

    while (ring.available() > 0) {
        REQUIRE_NOTHROW(
            ring.emplace_back(int{static_cast<int>(ring.available()) * 100}));
    }
    REQUIRE(ring.back() == 100);

    REQUIRE_NOTHROW(ring.pop_front(25));
    REQUIRE(ring.size() == 75);
    REQUIRE(ring.front() == 25);

    REQUIRE_NOTHROW(ring.clear());
    REQUIRE(ring.empty());
    REQUIRE(ring.available() == 100);
    REQUIRE(ring.capacity() == 100);
    REQUIRE(ring.size() == 0);

    std::vector<int> buffer;
    for (int i = 0; i < 50; i++) {
        buffer.push_back(i ^ 0xdead);
    }
    REQUIRE_NOTHROW(ring.insert(buffer.begin(), buffer.end()));
    REQUIRE(ring.size() == buffer.size());
    REQUIRE_NOTHROW(ring.push_back(100));
    std::vector<int> buffer2;
    REQUIRE_NOTHROW(ring.collect(ring.size() / 2, std::back_inserter(buffer2)));
    REQUIRE_NOTHROW(ring.collect(std::back_inserter(buffer2)));
    buffer.push_back(100);
    REQUIRE(buffer == buffer2);
    REQUIRE(ring.empty());
}

TEST_CASE("Dynamic ring buffer correctly stores elements") {
    SlokedRingBuffer<int, SlokedRingBufferType::Dynamic> ring(10);
    REQUIRE(ring.GetType() == SlokedRingBufferType::Dynamic);
    REQUIRE(ring.empty());
    REQUIRE(ring.size() == 0);

    REQUIRE_THROWS(ring.front());
    REQUIRE_THROWS(ring.back());
    REQUIRE_THROWS(ring.pop_front());
    REQUIRE_THROWS(ring.pop_front(10));
    REQUIRE_THROWS(ring.pop_front(100));
    REQUIRE_THROWS(ring.at(0));
    REQUIRE_THROWS(ring.at(50));
    REQUIRE_THROWS(ring.at(100));

    for (int i = 0; i < 20; i++) {
        REQUIRE_NOTHROW(ring.push_back(i));
        REQUIRE_NOTHROW(ring.emplace_back(int{i * 100}));
    }
    REQUIRE(ring.size() == 40);
    for (int i = 0; i < static_cast<int>(ring.size()); i++) {
        if (i % 2 == 0) {
            REQUIRE(ring.at(i) == i / 2);
        } else {
            REQUIRE(ring.at(i) == (i / 2) * 100);
        }
        REQUIRE(ring.at(i) == ring[i]);
    }
    while (ring.size() > 20) {
        REQUIRE_NOTHROW(ring.pop_front());
    }
    REQUIRE(ring.size() == 20);
    std::vector<int> buffer(10, 100);
    ring.insert(buffer.begin(), buffer.end());
    REQUIRE(ring.size() == 30);

    std::vector<int> buffer2(100, 1000);
    ring.insert(buffer2.begin(), buffer2.end());
    REQUIRE(ring.size() == 130);

    int sum{0};
    while (!ring.empty()) {
        sum += ring.front();
        REQUIRE_NOTHROW(ring.pop_front());
    }
    REQUIRE(sum == 115645);

    REQUIRE_NOTHROW(ring.reset());
    REQUIRE(ring.empty());
    REQUIRE(ring.size() == 0);
    REQUIRE(ring.capacity() == 0);

    REQUIRE_NOTHROW(ring.push_back(100));
    REQUIRE(ring.size() == 1);
    REQUIRE(ring.front() == 100);
}