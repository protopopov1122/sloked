#include "sloked/core/Position.h"

#include "catch2/catch.hpp"

using namespace sloked;

TEST_CASE("Text positions are correctly compared") {
    for (unsigned int i = 1; i <= 5; i++) {
        REQUIRE(TextPosition{i, i} == TextPosition{i, i});
        REQUIRE(TextPosition{i, i} >= TextPosition{i, i});
        REQUIRE(TextPosition{i, i} <= TextPosition{i, i});
        REQUIRE_FALSE(TextPosition{i, i} > TextPosition{i, i});
        REQUIRE_FALSE(TextPosition{i, i} < TextPosition{i, i});
        REQUIRE_FALSE(TextPosition{i, i} != TextPosition{i, i});

        REQUIRE(TextPosition{i, i + 1} > TextPosition{i, i});
        REQUIRE(TextPosition{i + 1, i} > TextPosition{i, i});
        REQUIRE(TextPosition{i + 1, i} > TextPosition{i, i + 10});
        REQUIRE(TextPosition{i, i + 1} >= TextPosition{i, i});
        REQUIRE(TextPosition{i + 1, i} >= TextPosition{i, i});
        REQUIRE(TextPosition{i + 1, i} >= TextPosition{i, i + 10});
        REQUIRE(TextPosition{i, i + 1} != TextPosition{i, i});
        REQUIRE(TextPosition{i + 1, i} != TextPosition{i, i});
        REQUIRE(TextPosition{i + 1, i} != TextPosition{i, i + 10});
        REQUIRE_FALSE(TextPosition{i, i + 1} < TextPosition{i, i});
        REQUIRE_FALSE(TextPosition{i + 1, i} < TextPosition{i, i});
        REQUIRE_FALSE(TextPosition{i + 1, i} < TextPosition{i, i + 10});
        REQUIRE_FALSE(TextPosition{i, i + 1} <= TextPosition{i, i});
        REQUIRE_FALSE(TextPosition{i + 1, i} <= TextPosition{i, i});
        REQUIRE_FALSE(TextPosition{i + 1, i} <= TextPosition{i, i + 10});
        REQUIRE_FALSE(TextPosition{i, i + 1} == TextPosition{i, i});
        REQUIRE_FALSE(TextPosition{i + 1, i} == TextPosition{i, i});
        REQUIRE_FALSE(TextPosition{i + 1, i} == TextPosition{i, i + 10});
    }
    REQUIRE(TextPosition::Min <= TextPosition::Max);
}

TEST_CASE("Text positions are correctly summed") {
    REQUIRE(TextPosition{1, 2} + TextPosition{3, 4} == TextPosition{4, 6});
    REQUIRE(TextPosition{10, 5} + TextPosition{67, 34} == TextPosition{77, 39});
    REQUIRE(TextPosition{15, 25} + TextPosition{35, 15} ==
            TextPosition{50, 40});
}

TEST_CASE("Text positions are correctly subtracted") {
    REQUIRE(TextPosition{1, 2} - TextPosition{1, 2} == TextPosition{0, 0});
    REQUIRE(TextPosition{10, 5} - TextPosition{5, 1} == TextPosition{5, 4});
    REQUIRE(TextPosition{100, 200} - TextPosition{50, 50} ==
            TextPosition{50, 150});
}

TEST_CASE("Text position delatas are correctly compared") {
    for (unsigned int i = 1; i <= 5; i++) {
        REQUIRE(TextPositionDelta{i, i} == TextPositionDelta{i, i});
        REQUIRE_FALSE(TextPositionDelta{i, i + 1} == TextPositionDelta{i, i});
        REQUIRE_FALSE(TextPositionDelta{i + 1, i} == TextPositionDelta{i, i});
        REQUIRE_FALSE(TextPositionDelta{i + 1, i} ==
                      TextPositionDelta{i, i + 10});
    }
}