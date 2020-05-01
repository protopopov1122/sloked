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

#include "sloked/core/AVL.h"

#include <iostream>

#include "catch2/catch.hpp"

using namespace sloked;

template <typename T>
class TestAVLNode : public AVLNode<TestAVLNode<T>> {
 public:
    TestAVLNode(T &&value, std::unique_ptr<TestAVLNode<T>> left,
                std::unique_ptr<TestAVLNode<T>> right)
        : AVLNode<TestAVLNode<T>>(std::move(left), std::move(right)),
          value(std::forward<T>(value)) {}

    const T &Value() const {
        return this->value;
    }

    TestAVLNode<T> *Left() const {
        return this->begin.get();
    }

    TestAVLNode<T> *Right() const {
        return this->end.get();
    }

    bool Balanced() {
        return this->AvlBalanced();
    }

    std::size_t GetHeight() const {
        std::size_t height = 0;
        if (this->begin) {
            height = std::max(height, this->begin->GetHeight() + 1);
        }
        if (this->end) {
            height = std::max(height, this->end->GetHeight() + 1);
        }
        return height;
    }

    void Insert(T &&value) {
        if (value < this->value) {
            if (this->begin) {
                this->begin->Insert(std::forward<T>(value));
            } else {
                this->begin = std::make_unique<TestAVLNode<T>>(
                    std::forward<T>(value), nullptr, nullptr);
            }
        } else if (value > this->value) {
            if (this->end) {
                this->end->Insert(std::forward<T>(value));
            } else {
                this->end = std::make_unique<TestAVLNode<T>>(
                    std::forward<T>(value), nullptr, nullptr);
            }
        }
        this->AvlBalance();
    }

 protected:
    void AvlUpdate() final {}

    void AvlSwapContent(TestAVLNode<T> &other) final {
        std::swap(this->value, other.value);
    }

 private:
    T value;
};

template <typename T>
std::unique_ptr<TestAVLNode<T>> MakeNode(
    T &&value, std::unique_ptr<TestAVLNode<T>> left = nullptr,
    std::unique_ptr<TestAVLNode<T>> right = nullptr) {
    return std::make_unique<TestAVLNode<T>>(std::forward<T>(value),
                                            std::move(left), std::move(right));
}

TEST_CASE("AVL correctly detects unbalanced trees") {
    auto tree1 = MakeNode<int>(100, nullptr, nullptr);
    REQUIRE(tree1->Balanced());

    auto tree2 = MakeNode<int>(100, MakeNode<int>(0), nullptr);
    REQUIRE(tree2->Balanced());
    auto tree3 = MakeNode<int>(100, nullptr, MakeNode<int>(200));
    REQUIRE(tree3->Balanced());
    auto tree4 = MakeNode<int>(100, MakeNode<int>(0), MakeNode<int>(200));
    REQUIRE(tree4->Balanced());

    auto tree5 =
        MakeNode<int>(100, MakeNode<int>(0, MakeNode<int>(-100)), nullptr);
    REQUIRE_FALSE(tree5->Balanced());
    auto tree6 = MakeNode<int>(
        100, MakeNode<int>(0, MakeNode<int>(-100), MakeNode<int>(50)), nullptr);
    REQUIRE_FALSE(tree6->Balanced());
    auto tree7 = MakeNode<int>(
        100, nullptr,
        MakeNode<int>(200, MakeNode<int>(150), MakeNode<int>(250)));
    REQUIRE_FALSE(tree7->Balanced());

    auto tree8 = MakeNode<int>(
        100, MakeNode<int>(0),
        MakeNode<int>(200, MakeNode<int>(150), MakeNode<int>(250)));
    REQUIRE(tree8->Balanced());
    auto tree9 = MakeNode<int>(
        100, MakeNode<int>(0, MakeNode<int>(-100), MakeNode<int>(50)),
        MakeNode<int>(200));
    REQUIRE(tree9->Balanced());
    auto tree10 = MakeNode<int>(
        100, MakeNode<int>(0, MakeNode<int>(-100), MakeNode<int>(50)),
        MakeNode<int>(200, MakeNode<int>(150), MakeNode<int>(250)));
    REQUIRE(tree10->Balanced());
}

TEST_CASE("AVL correctly balances trees") {
    auto root = MakeNode<int>(0);
    for (std::size_t i = 1; i < 10; i++) {
        root->Insert(i);
    }
    REQUIRE(root->Balanced());
    REQUIRE(root->GetHeight() == 3);

    REQUIRE(root->Value() == 3);
    REQUIRE(root->Left() != nullptr);
    REQUIRE(root->Left()->Value() == 1);
    REQUIRE(root->Left()->Left() != nullptr);
    REQUIRE(root->Left()->Left()->Value() == 0);
    REQUIRE(root->Left()->Left()->Left() == nullptr);
    REQUIRE(root->Left()->Left()->Right() == nullptr);
    REQUIRE(root->Left()->Right() != nullptr);
    REQUIRE(root->Left()->Right()->Value() == 2);
    REQUIRE(root->Left()->Right()->Left() == nullptr);
    REQUIRE(root->Left()->Right()->Right() == nullptr);
    REQUIRE(root->Right() != nullptr);
    REQUIRE(root->Right()->Value() == 7);
    REQUIRE(root->Right()->Left() != nullptr);
    REQUIRE(root->Right()->Left()->Value() == 5);
    REQUIRE(root->Right()->Left()->Left() != nullptr);
    REQUIRE(root->Right()->Left()->Left()->Value() == 4);
    REQUIRE(root->Right()->Left()->Left()->Left() == nullptr);
    REQUIRE(root->Right()->Left()->Left()->Right() == nullptr);
    REQUIRE(root->Right()->Left()->Right() != nullptr);
    REQUIRE(root->Right()->Left()->Right()->Value() == 6);
    REQUIRE(root->Right()->Left()->Right()->Left() == nullptr);
    REQUIRE(root->Right()->Left()->Right()->Right() == nullptr);
    REQUIRE(root->Right()->Right() != nullptr);
    REQUIRE(root->Right()->Right()->Value() == 8);
    REQUIRE(root->Right()->Right()->Left() == nullptr);
    REQUIRE(root->Right()->Right()->Right() != nullptr);
    REQUIRE(root->Right()->Right()->Right()->Value() == 9);
    REQUIRE(root->Right()->Right()->Right()->Left() == nullptr);
    REQUIRE(root->Right()->Right()->Right()->Right() == nullptr);
}