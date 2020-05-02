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

#include "sloked/core/Listener.h"

#include "catch2/catch.hpp"

using namespace sloked;

class TestListener {
 public:
    TestListener(std::function<void(int)> callback)
        : callback(std::move(callback)) {}

    void Trigger(const int &value) {
        if (this->callback) {
            this->callback(value);
        }
    }

 private:
    std::function<void(int)> callback;
};

class TestListenerManager
    : public SlokedListenerManager<TestListener, int, void> {
 public:
    void Trigger(int value) {
        this->TriggerListeners(&TestListener::Trigger, value);
    }
};

TEST_CASE("Listener manager invokes attached listeners") {
    bool results[] = {false, false};
    TestListenerManager manager;
    auto listener1 =
        std::make_shared<TestListener>([&results](const auto &value) {
            REQUIRE_FALSE(results[0]);
            results[0] = true;
        });
    auto listener2 =
        std::make_shared<TestListener>([&results](const auto &value) {
            REQUIRE_FALSE(results[1]);
            results[1] = true;
        });
    auto listener3 =
        std::make_shared<TestListener>([&results](const auto &value) {
            REQUIRE(false);  // Fail
        });
    manager.AddListener(listener1);
    manager.AddListener(listener2);
    manager.AddListener(listener3);
    REQUIRE_FALSE(results[0]);
    REQUIRE_FALSE(results[1]);
    manager.RemoveListener(*listener3);
    manager.Trigger(0);
    REQUIRE(results[0]);
    REQUIRE(results[1]);
    manager.ClearListeners();
    manager.Trigger(0);
}