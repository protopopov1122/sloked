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

#include "sloked/kgr/Value.h"

#include "catch2/catch.hpp"

using namespace sloked;

static void TestWrongType(const KgrValue &value) {
    if (value.GetType() != KgrValueType::Null) {
        REQUIRE_FALSE(value.Is(KgrValueType::Null));
    }
    if (value.GetType() != KgrValueType::Integer) {
        REQUIRE_FALSE(value.Is(KgrValueType::Integer));
        REQUIRE_THROWS(value.AsInt());
    }
    if (value.GetType() != KgrValueType::Number) {
        REQUIRE_FALSE(value.Is(KgrValueType::Number));
        REQUIRE_THROWS(value.AsNumber());
    }
    if (value.GetType() != KgrValueType::Boolean) {
        REQUIRE_FALSE(value.Is(KgrValueType::Boolean));
        REQUIRE_THROWS(value.AsBoolean());
    }
    if (value.GetType() != KgrValueType::String) {
        REQUIRE_FALSE(value.Is(KgrValueType::String));
        REQUIRE_THROWS(value.AsString());
    }
    if (value.GetType() != KgrValueType::Array) {
        REQUIRE_FALSE(value.Is(KgrValueType::Array));
        REQUIRE_THROWS(value.AsArray());
    }
    if (value.GetType() != KgrValueType::Object) {
        REQUIRE_FALSE(value.Is(KgrValueType::Object));
        REQUIRE_THROWS(value.AsDictionary());
    }
}

static void RequireEqual(const KgrValue &value1, const KgrValue &value2) {
    REQUIRE(value1.GetType() == value2.GetType());
    switch (value1.GetType()) {
        case KgrValueType::Null:
            break;
        
        case KgrValueType::Integer:
            REQUIRE(value1.AsInt() == value2.AsInt());
            break;

        case KgrValueType::Number:
            REQUIRE(value1.AsNumber() == Approx(value2.AsNumber()));
            break;

        case KgrValueType::Boolean:
            REQUIRE(value1.AsBoolean() == value2.AsBoolean());
            break;

        case KgrValueType::String:
            REQUIRE(value1.AsString() == value2.AsString());
            break;

        case KgrValueType::Array: {
            const auto &arr1 = value1.AsArray();
            const auto &arr2 = value2.AsArray();
            REQUIRE(arr1.Size() == arr2.Size());
            for (std::size_t i = 0; i < arr1.Size(); i++) {
                RequireEqual(arr1.At(i), arr2.At(i));
            }
        } break;

        case KgrValueType::Object: {
            const auto &obj1 = value1.AsDictionary();
            const auto &obj2 = value2.AsDictionary();
            REQUIRE(obj1.Size() == obj2.Size());
            for (const auto &kv : obj1) {
                REQUIRE(obj2.Has(kv.first));
                RequireEqual(kv.second, obj2[kv.first]);
            }
        } break;
    }
}

TEST_CASE("Value correctly stores null values") {
    KgrValue value;
    REQUIRE(value.GetType() == KgrValueType::Null);
    TestWrongType(value);
}

TEST_CASE("Value correctly stores integers") {
    int64_t ivalue = GENERATE(-100, -42, -15, -8, 0, 1, 14, 22, 67, 100);
    KgrValue value{ivalue};
    REQUIRE(value.GetType() == KgrValueType::Integer);
    REQUIRE(value.AsInt() == ivalue);
    TestWrongType(value);
}

TEST_CASE("Value correctly stores numbers") {
    double dvalue = GENERATE(-31.23, -5.43, -0.06, 0, 0.0051, 0.76, 2.71, 3.14, 100.456);
    KgrValue value{dvalue};
    REQUIRE(value.GetType() == KgrValueType::Number);
    REQUIRE(value.AsNumber() == Approx(dvalue));
    TestWrongType(value);
}

TEST_CASE("Value correctly stores booleans") {
    bool bvalue = GENERATE(true, false);
    KgrValue value{bvalue};
    REQUIRE(value.GetType() == KgrValueType::Boolean);
    REQUIRE(value.AsBoolean() == bvalue);
    TestWrongType(value);
}

TEST_CASE("Value correctly stores strings") {
    const std::string &svalue = GENERATE_REF("", "Hello, world!", "\t\n\t\n\n\n\n\t", "TEST..test");
    SECTION("By reference") {
        KgrValue value{svalue};
        REQUIRE(value.GetType() == KgrValueType::String);
        REQUIRE(value.AsString() == svalue);
        TestWrongType(value);
    }
    SECTION("By view") {
        KgrValue value{std::string_view{svalue}};
        REQUIRE(value.GetType() == KgrValueType::String);
        REQUIRE(value.AsString() == svalue);
        TestWrongType(value);
    }
    SECTION("By pointer") {
        KgrValue value{svalue.c_str()};
        REQUIRE(value.GetType() == KgrValueType::String);
        REQUIRE(value.AsString() == svalue);
        TestWrongType(value);
    }
}

TEST_CASE("Array correctly stores values") {
    SECTION("Empty array") {
        KgrArray array;
        REQUIRE(array.Empty());
        REQUIRE(array.Size() == 0);
        REQUIRE_THROWS(array.At(0));
        REQUIRE_THROWS(array[0]);
        REQUIRE(std::distance(array.begin(), array.end()) == 0);
        REQUIRE(std::distance(array.rbegin(), array.rend()) == 0);
        REQUIRE_THROWS(array.Replace(0, KgrValue{}));
        REQUIRE_THROWS(array.Replace(0, array));
        REQUIRE_THROWS(array.Insert(1, KgrValue{}));
        REQUIRE_THROWS(array.Insert(1, array));
        REQUIRE_THROWS(array.Remove(0));
        REQUIRE_NOTHROW(array.Append(KgrValue{1}));
        REQUIRE_NOTHROW(array.Insert(0, KgrValue{2}));
        REQUIRE_FALSE(array.Empty());
        REQUIRE(array.Size() == 2);
        REQUIRE(array.At(0).AsInt() == 2);
        REQUIRE(array.At(1).AsInt() == 1);
        REQUIRE(array[0].AsInt() == 2);
        REQUIRE(array[1].AsInt() == 1);
        REQUIRE(std::distance(array.begin(), array.end()) == 2);
        REQUIRE(std::distance(array.rbegin(), array.rend()) == 2);
    }
    SECTION("From std::vector and std::initializer_list") {
        auto array = GENERATE(KgrArray(std::vector<KgrValue> {
            5, true, "Hello"
        }), KgrArray{5, true, "Hello"});
        REQUIRE_FALSE(array.Empty());
        REQUIRE(array.Size() == 3);
        REQUIRE(array.At(0).AsInt() == 5);
        REQUIRE(array[0].AsInt() == 5);
        REQUIRE(array.At(1).AsBoolean());
        REQUIRE(array[1].AsBoolean());
        REQUIRE(array.At(2).AsString() == "Hello");
        REQUIRE(array[2].AsString() == "Hello");
        REQUIRE_NOTHROW(array.Replace(0, {}));
        REQUIRE(array.At(0).Is(KgrValueType::Null));
        REQUIRE_NOTHROW(array.Remove(1));
        REQUIRE_NOTHROW(array.Append(3.14));
        REQUIRE(array.Size() == 3);
        REQUIRE(std::next(array.begin(), 2)->AsNumber() == Approx(3.14));
    }
}

TEST_CASE("Value correctly stores arrays") {
    auto avalue = GENERATE(KgrArray{1, 2, 3, 4, 5}, KgrArray{"Hello, ", "cruel", 50}, KgrArray{true, false, 5.34, KgrArray{1, 2, 3}});
    KgrValue value{avalue};
    REQUIRE(value.GetType() == KgrValueType::Array);
    RequireEqual(value.AsArray(), avalue);
    TestWrongType(value);
}

TEST_CASE("Object correcty stores values") {
    SECTION("Empty dictionary") {
        KgrDictionary dict;
        REQUIRE(dict.Empty());
        REQUIRE(dict.Size() == 0);
        REQUIRE_FALSE(dict.Has("a1"));
        REQUIRE_THROWS(dict.Get("a1"));
        REQUIRE_THROWS(dict["a1"]);
        REQUIRE(std::distance(dict.begin(), dict.end()) == 0);
        REQUIRE(std::distance(dict.rbegin(), dict.rend()) == 0);
        REQUIRE_NOTHROW(dict.Put("a1", {}));
        REQUIRE_NOTHROW(dict.Put("a1", 10));
        REQUIRE_NOTHROW(dict.Put("a2", true));
        REQUIRE_NOTHROW(dict.Put("a3", "Hello"));
        REQUIRE_NOTHROW(dict.Remove("a2"));
        REQUIRE_THROWS(dict.Remove("a2"));
        REQUIRE(dict.Size() == 2);
        REQUIRE(dict.Has("a1"));
        REQUIRE_FALSE(dict.Has("a2"));
        REQUIRE(dict.Has("a3"));
        REQUIRE(dict.Get("a1").AsInt() == 10);
        REQUIRE(dict["a1"].AsInt() == 10);
        REQUIRE_THROWS(dict.Get("a2"));
        REQUIRE_THROWS(dict["a2"]);
        REQUIRE(dict.Get("a3").AsString() == "Hello");
        REQUIRE(dict["a3"].AsString() == "Hello");
        REQUIRE(std::distance(dict.begin(), dict.end()) == 2);
        REQUIRE(std::distance(dict.rbegin(), dict.rend()) == 2);
    }
    SECTION("From std::map and std::initializer_list") {
        auto dict = GENERATE(KgrDictionary(std::map<std::string, KgrValue>{
            { "a1", 5 },
            { "a2", true },
            { "a3", {} }
        }), KgrDictionary{
            { "a1", 5 },
            { "a2", true },
            { "a3", {} }
        });
        REQUIRE_FALSE(dict.Empty());
        REQUIRE(dict.Size() == 3);
        REQUIRE(std::distance(dict.begin(), dict.end()) == 3);
        REQUIRE(std::distance(dict.rbegin(), dict.rend()) == 3);
        REQUIRE(dict.Has("a1"));
        REQUIRE(dict.Has("a2"));
        REQUIRE(dict.Has("a3"));
        REQUIRE_FALSE(dict.Has("a4"));
        REQUIRE(dict.Get("a1").AsInt() == 5);
        REQUIRE(dict["a1"].AsInt() == 5);
        REQUIRE(dict.Get("a2").AsBoolean());
        REQUIRE(dict["a2"].AsBoolean());
        REQUIRE(dict.Get("a3").Is(KgrValueType::Null));
        REQUIRE(dict["a3"].Is(KgrValueType::Null));
    }
}