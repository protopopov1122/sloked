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

#include "catch2/catch.hpp"
#include "sloked/core/OrderedCache.h"

using namespace sloked;

TEST_CASE("Cache fetches specified range from supplier") {
    constexpr int offset = 10;
    SlokedOrderedCache<int, int> cache([](const auto &from, const auto &to) {
        std::vector<int> result;
        for (int i = from; i <= to; i++) {
            result.push_back(i + offset);
        }
        return result;
    });
    int current = 10;
    constexpr int end = 100;
    auto range = cache.Fetch(current, end);
    REQUIRE(std::distance(range.first, range.second) == end - current + 1);
    for (auto it = range.first; it != range.second; ++it, ++current) {
        REQUIRE(it->second == current + offset);
    }
}

TEST_CASE("Cache throws on reversed requested range") {
    SlokedOrderedCache<int, int> cache([](const auto &from, const auto &to) {
        return std::vector<int>(to - from + 1, 0);
    });
    REQUIRE_THROWS(cache.Fetch(100, 10));
    REQUIRE_THROWS(cache.FetchUpdated(100, 10));
    REQUIRE_NOTHROW(cache.Fetch(0, 0));
    REQUIRE_NOTHROW(cache.FetchUpdated(0, 0));
}

TEST_CASE("Cache throws on incorrect supply result range") {
    SECTION("Less than expected") {
        SlokedOrderedCache<int, int> cache(
            [](const auto &from, const auto &to) {
                return std::vector<int>{};
            });
        REQUIRE_THROWS(cache.Fetch(1, 10));
    }
    SECTION("More than expected") {
        SlokedOrderedCache<int, int> cache(
            [](const auto &from, const auto &to) {
                return std::vector<int>(to - from + 10, 0);
            });
        REQUIRE_THROWS(cache.Fetch(1, 10));
    }
}

TEST_CASE("Cache does not duplicate supplier invocations") {
    int fetched = 0;
    SlokedOrderedCache<int, int> cache(
        [&fetched](const auto &from, const auto &to) {
            int count = to - from + 1;
            fetched += count;
            return std::vector<int>(count, 0);
        });
    cache.Fetch(1, 10);
    cache.Drop(8, 15);
    cache.Fetch(1, 15);
    REQUIRE(fetched == 18);
}

TEST_CASE("Cache is able to fetch only updated parts") {
    SlokedOrderedCache<int, int> cache([](const auto &from, const auto &to) {
        return std::vector<int>(to - from + 1, 0);
    });
    cache.Fetch(1, 100);
    cache.Drop(25, 30);
    cache.Drop(35, 50);
    auto updated = cache.FetchUpdated(27, 45);
    REQUIRE(updated.size() == 15);
    for (const auto &entry : updated) {
        REQUIRE(((entry.first >= 27 && entry.first <= 30) ||
                 (entry.first >= 35 && entry.first <= 45)));
    }
}

TEST_CASE("Cache is able to clear itself") {
    int fetched = 0;
    SlokedOrderedCache<int, int> cache(
        [&fetched](const auto &from, const auto &to) {
            int count = to - from + 1;
            fetched += count;
            return std::vector<int>(count, 0);
        });
    cache.Fetch(1, 100);
    REQUIRE(fetched == 100);
    cache.Fetch(1, 100);
    REQUIRE(fetched == 100);
    cache.Clear();
    cache.Fetch(1, 100);
    REQUIRE(fetched == 200);
}

TEST_CASE("Cache supports insertion without calling supplier") {
    SlokedOrderedCache<int, int> cache(
        [](const auto &from, const auto &to) -> std::vector<int> {
            throw std::exception{};
        });
    std::map<int, int> values{{1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}};
    REQUIRE_THROWS(cache.Fetch(2, 4));
    cache.Insert(values.begin(), values.end());
    auto fetched = cache.Fetch(2, 4);
    REQUIRE(std::distance(fetched.first, fetched.second) == 3);
    REQUIRE(fetched.first->second == 2);
    REQUIRE(std::next(fetched.first, 1)->second == 3);
    REQUIRE(std::next(fetched.first, 2)->second == 4);
    REQUIRE_THROWS(cache.Fetch(2, 6));
    REQUIRE_NOTHROW(cache.Fetch(1, 5));
}