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

#include "sloked/kgr/Pipe.h"
#include "sloked/kgr/local/Pipe.h"

#include "catch2/catch.hpp"

using namespace sloked;

static void TestPipeState(std::unique_ptr<KgrPipe> pipe1, std::unique_ptr<KgrPipe> pipe2) {
    REQUIRE(pipe1->GetStatus() == KgrPipe::Status::Open);
    REQUIRE(pipe2->GetStatus() == KgrPipe::Status::Open);
    REQUIRE(pipe1->Empty());
    REQUIRE(pipe1->Count() == 0);
    REQUIRE(pipe2->Empty());
    REQUIRE(pipe2->Count() == 0);

    pipe1->Write({});
    REQUIRE_FALSE(pipe2->Empty());
    REQUIRE(pipe2->Count() == 1);
    pipe1->Write({});
    pipe1->Write({});
    REQUIRE_FALSE(pipe2->Empty());
    REQUIRE(pipe2->Count() == 3);
    REQUIRE(pipe1->Empty());
    REQUIRE(pipe1->Count() == 0);

    pipe2->Write({});
    REQUIRE_FALSE(pipe1->Empty());
    REQUIRE(pipe1->Count() == 1);
    REQUIRE_FALSE(pipe2->Empty());
    REQUIRE(pipe2->Count() == 3);

    REQUIRE_NOTHROW(pipe2->Drop(2));
    REQUIRE_FALSE(pipe1->Empty());
    REQUIRE(pipe1->Count() == 1);
    REQUIRE_FALSE(pipe2->Empty());
    REQUIRE(pipe2->Count() == 1);

    REQUIRE_NOTHROW(pipe1->Close());
    REQUIRE(pipe1->GetStatus() == KgrPipe::Status::Closed);
    REQUIRE(pipe2->GetStatus() == KgrPipe::Status::Closed);
    REQUIRE_FALSE(pipe1->Empty());
    REQUIRE(pipe1->Count() == 1);
    REQUIRE_FALSE(pipe2->Empty());
    REQUIRE(pipe2->Count() == 1);

    REQUIRE_NOTHROW(pipe1->DropAll());
    REQUIRE_NOTHROW(pipe2->DropAll());
    REQUIRE(pipe1->GetStatus() == KgrPipe::Status::Closed);
    REQUIRE(pipe2->GetStatus() == KgrPipe::Status::Closed);
    REQUIRE(pipe1->Empty());
    REQUIRE(pipe1->Count() == 0);
    REQUIRE(pipe2->Empty());
    REQUIRE(pipe2->Count() == 0);
}

static void TestPipeTransport(std::unique_ptr<KgrPipe> pipe1, std::unique_ptr<KgrPipe> pipe2) {
    std::thread helper([&pipe2] {
        REQUIRE_NOTHROW(pipe2->Wait(3));
        REQUIRE(pipe2->Count() == 3);
        auto value = pipe2->Read();
        REQUIRE(value.Is(KgrValueType::Integer));
        REQUIRE(value.AsInt() == 100);
        value = pipe2->ReadWait();
        REQUIRE(value.Is(KgrValueType::Number));
        REQUIRE(value.AsNumber() == Approx(3.14));
        value = pipe2->ReadOptional().value();
        REQUIRE(value.Is(KgrValueType::Boolean));
        REQUIRE(value.AsBoolean());
        REQUIRE(pipe2->Empty());
        REQUIRE_THROWS(pipe2->Read());
        REQUIRE_FALSE(pipe2->ReadOptional().has_value());

        pipe2->Write("Hello, world!");
        pipe2->Wait(10);
        REQUIRE(pipe2->Count() >= 10);
        pipe2->Wait(20);
        REQUIRE_NOTHROW(pipe2->Drop(pipe2->Count() - 10));
        REQUIRE(pipe2->Count() == 10);
        REQUIRE_NOTHROW(pipe2->Drop(5));
        REQUIRE(pipe2->Count() == 5);
        REQUIRE_NOTHROW(pipe2->DropAll());
        REQUIRE(pipe2->Empty());

        pipe2->Write(KgrArray {
            1, 2, 3, 4, 5
        });
        value = pipe2->ReadWait();
        REQUIRE(value.Is(KgrValueType::Object));
        const auto &dict = value.AsDictionary();
        REQUIRE(dict.Size() == 5);
        REQUIRE(dict.Has("1"));
        REQUIRE(dict["1"].Is(KgrValueType::Integer));
        REQUIRE(dict["1"].AsInt() == 11);
        REQUIRE(dict.Has("2"));
        REQUIRE(dict["2"].Is(KgrValueType::Integer));
        REQUIRE(dict["2"].AsInt() == 12);
        REQUIRE(dict.Has("3"));
        REQUIRE(dict["3"].Is(KgrValueType::Integer));
        REQUIRE(dict["3"].AsInt() == 13);
        REQUIRE(dict.Has("4"));
        REQUIRE(dict["4"].Is(KgrValueType::Integer));
        REQUIRE(dict["4"].AsInt() == 14);
        REQUIRE(dict.Has("5"));
        REQUIRE(dict["5"].Is(KgrValueType::Integer));
        REQUIRE(dict["5"].AsInt() == 15);
        pipe2->Write({});
    });

    REQUIRE_NOTHROW(pipe1->Write(100));
    REQUIRE(pipe1->SafeWrite(3.14));
    REQUIRE_NOTHROW(pipe1->Write(true));
    auto value = pipe1->ReadWait();
    REQUIRE(value.Is(KgrValueType::String));
    REQUIRE(value.AsString() == "Hello, world!");
    REQUIRE(pipe1->Empty());

    for (int i = 0; i < 20; i++) {
        pipe1->Write(i);
    }
    value = pipe1->ReadWait();
    REQUIRE(value.Is(KgrValueType::Array));
    const auto &arr = value.AsArray();
    REQUIRE(arr.Size() == 5);
    KgrDictionary dict;
    for (const auto &el : arr) {
        dict.Put(std::to_string(el.AsInt()), el.AsInt() + 10);
    }
    pipe1->Write(std::move(dict));

    helper.join();
    REQUIRE(pipe1->ReadWait().Is(KgrValueType::Null));
}

TEST_CASE("Local pipe correctly reports state") {
    auto [pipe1, pipe2] = KgrLocalPipe::Make();
    TestPipeState(std::move(pipe1), std::move(pipe2));
}

static void TestPipeNotifications(std::unique_ptr<KgrPipe> pipe1, std::unique_ptr<KgrPipe> pipe2) {
    int expectedValue{0};
    pipe2->SetMessageListener([&pipe2, &expectedValue] {
        if (pipe2->GetStatus() == KgrPipe::Status::Open) {
            auto value = pipe2->Read();
            REQUIRE(value.AsInt() == expectedValue);
            pipe2->Write(value.AsInt() * 100);
        } else {
            REQUIRE(pipe2->Empty());
        }
    });
    auto send = [&](int num) {
        expectedValue = num;
        pipe1->Write(expectedValue);
        auto value = pipe1->ReadWait();
        REQUIRE(value.AsInt() == expectedValue * 100);
    };
    send(10);
    send(15);
    send(100);
    send(-5);
    pipe1.reset();
}

TEST_CASE("Local pipe correctly transports values") {
    auto [pipe1, pipe2] = KgrLocalPipe::Make();
    TestPipeTransport(std::move(pipe1), std::move(pipe2));
}

TEST_CASE("Local pipe notifies on new message arrival") {
    auto [pipe1, pipe2] = KgrLocalPipe::Make();
    TestPipeNotifications(std::move(pipe1), std::move(pipe2));
}