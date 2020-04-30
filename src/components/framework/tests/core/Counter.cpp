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

#include "sloked/core/Counter.h"

#include <condition_variable>
#include <mutex>
#include <thread>

#include "catch2/catch.hpp"

using namespace sloked;

TEST_CASE("Counter invokes callback on modification") {
    constexpr int Count = 10, Init = 1;
    SlokedCounter<int> counter{Init};
    bool started{false}, flag{false};
    std::mutex mtx;
    std::condition_variable cv;
    std::vector<int> counterHist;
    std::thread helper([&] {
        counter.Wait([&](auto count) {
            std::unique_lock lock(mtx);
            started = true;
            counterHist.push_back(count);
            flag = true;
            cv.notify_one();
            return count == 0;
        });
    });
    std::unique_lock lock(mtx);
    while (!started) {
        cv.wait(lock);
    }
    for (int i = 0; i < Count; i++) {
        lock.unlock();
        counter.Increment();
        lock.lock();
        while (!flag) {
            cv.wait(lock);
        }
        flag = false;
    }
    for (int i = 0; i < Count; i++) {
        lock.unlock();
        counter.Decrement();
        lock.lock();
        while (!flag) {
            cv.wait(lock);
        }
        flag = false;
    }
    counter.Decrement();

    lock.unlock();
    helper.join();
    REQUIRE(counterHist == std::vector<int>{1,  1, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                                            10, 9, 8, 7, 6, 5, 4, 3, 2, 1,  0});
}

TEST_CASE("Counter handler modify counter") {
    SlokedCounter<int> counter;
    SlokedCounter<int>::Handle handle{counter}, handle2{counter},
        handle3{counter};
    SlokedCounter<int>::Handle handle4 = handle;
    SlokedCounter<int>::Handle handle5 = std::move(handle2);
    std::vector<int> counterHist;
    std::mutex mtx;
    std::condition_variable cv;
    bool started = false;
    std::thread helper([&] {
        counter.Wait([&](auto count) {
            std::unique_lock lock(mtx);
            started = true;
            counterHist.push_back(count);
            cv.notify_one();
            return count == 0;
        });
    });
    std::unique_lock lock(mtx);
    while (!started) {
        cv.wait(lock);
    }
    handle.Reset();
    cv.wait(lock);
    handle2.Reset();
    handle3.Reset();
    cv.wait(lock);
    handle4.Reset();
    cv.wait(lock);
    handle5.Reset();
    cv.wait(lock);
    helper.join();
    REQUIRE(counterHist == std::vector<int>{4, 4, 3, 2, 1, 0});
}