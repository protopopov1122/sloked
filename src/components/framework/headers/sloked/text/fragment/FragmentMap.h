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

#ifndef SLOKED_TEXT_FRAGMENT_FRAGMENTMAP_H_
#define SLOKED_TEXT_FRAGMENT_FRAGMENTMAP_H_

#include <algorithm>
#include <optional>

#include "sloked/core/AVL.h"
#include "sloked/core/Error.h"
#include "sloked/text/fragment/TextFragment.h"

namespace sloked {

    template <typename T>
    class TaggedFragmentMapNode : public AVLNode<TaggedFragmentMapNode<T>> {
     public:
        TaggedFragmentMapNode(TaggedTextFragment<T> &&fragment)
            : AVLNode<TaggedFragmentMapNode<T>>(nullptr, nullptr),
              content(std::forward<TaggedTextFragment<T>>(fragment)), height(0),
              empty(false) {}

        std::size_t GetHeight() const {
            return this->height;
        }

        bool Empty() const {
            return this->empty;
        }

        bool Has(const TextPosition &pos) {
            return (this->content.has_value() &&
                    this->content.value().Includes(pos)) ||
                   (this->begin && this->begin->Has(pos)) ||
                   (this->end && this->end->Has(pos));
        }

        const TaggedTextFragment<T> *Minimum() const {
            const TaggedTextFragment<T> *res =
                this->content.has_value() ? &this->content.value() : nullptr;
            if (this->begin) {
                const TaggedTextFragment<T> *bres = this->begin->Minimum();
                if (bres) {
                    res = bres;
                }
            }
            return res;
        }

        const TaggedTextFragment<T> *Maximum() const {
            const TaggedTextFragment<T> *res =
                this->content.has_value() ? &this->content.value() : nullptr;
            if (this->end) {
                const TaggedTextFragment<T> *eres = this->end->Maximum();
                if (eres) {
                    res = eres;
                }
            }
            return res;
        }

        const TaggedTextFragment<T> *Get(const TextPosition &pos) {
            const TaggedTextFragment<T> *fragment = nullptr;
            if (this->content.has_value() &&
                this->content.value().Includes(pos)) {
                fragment = &this->content.value();
            }
            if (fragment == nullptr) {
                if (this->content.has_value()) {
                    if (this->begin && pos < this->content.value().GetStart()) {
                        fragment = this->begin->Get(pos);
                    } else if (this->end) {
                        fragment = this->end->Get(pos);
                    }
                } else {
                    if (this->begin) {
                        fragment = this->begin->Get(pos);
                    }
                    if (fragment == nullptr && this->end) {
                        fragment = this->end->Get(pos);
                    }
                }
            }
            return fragment;
        }

        template <typename I>
        I GetLine(TextPosition::Line line, I iter) {
            if (this->begin &&
                (!this->content.has_value() ||
                 this->content.value().GetStart().line >= line)) {
                iter = this->begin->GetLine(line, std::move(iter));
            }
            if (this->content.has_value() &&
                this->content.value().Includes(line)) {
                iter++ = this->content.value();
            }
            if (this->end && (!this->content.has_value() ||
                              this->content.value().GetStart().line <= line)) {
                iter = this->end->GetLine(line, std::move(iter));
            }
            return iter;
        }

        void Insert(TaggedTextFragment<T> &&fragment) {
            if (this->content.has_value()) {
                if (this->content.value().Overlaps(fragment)) {
                    throw SlokedError("Given fragment overlaps existing");
                }
                if (fragment < this->content) {
                    if (this->begin) {
                        this->begin->Insert(
                            std::forward<TaggedTextFragment<T>>(fragment));
                    } else {
                        this->begin =
                            std::make_unique<TaggedFragmentMapNode<T>>(
                                std::forward<TaggedTextFragment<T>>(fragment));
                    }
                } else {
                    if (this->end) {
                        this->end->Insert(
                            std::forward<TaggedTextFragment<T>>(fragment));
                    } else {
                        this->end = std::make_unique<TaggedFragmentMapNode<T>>(
                            std::forward<TaggedTextFragment<T>>(fragment));
                    }
                }
            } else {
                auto max_begin = this->begin ? this->begin->Maximum() : nullptr;
                auto min_end = this->end ? this->end->Minimum() : nullptr;
                if (max_begin && fragment < *max_begin) {
                    this->begin->Insert(
                        std::forward<TaggedTextFragment<T>>(fragment));
                } else if (min_end && *min_end < fragment) {
                    this->end->Insert(
                        std::forward<TaggedTextFragment<T>>(fragment));
                } else {
                    this->content =
                        std::forward<TaggedTextFragment<T>>(fragment);
                }
            }
            this->UpdateStats();
            if (!this->AvlBalanced()) {
                this->AvlBalance();
            }
        }

        std::optional<TextPosition> Remove(const TextPosition &pos) {
            std::optional<TextPosition> res;
            if (this->content.has_value()) {
                auto start = this->content.value().GetStart();
                auto len = this->content.value().GetLength();
                TextPosition end{start.line + len.line,
                                 start.column + len.column};
                if (pos < start || pos == start || (start < pos && pos < end)) {
                    this->content.reset();
                    res = start;
                }
            }
            if (this->begin) {
                auto beginRes = this->begin->Remove(pos);
                if (beginRes.has_value() &&
                    (!res.has_value() || beginRes.value() < res.value())) {
                    res = beginRes;
                }
            }
            if (this->end) {
                auto endRes = this->end->Remove(pos);
                if (endRes.has_value() &&
                    (!res.has_value() || endRes.value() < res.value())) {
                    res = endRes;
                }
            }
            this->UpdateStats();
            if (!this->AvlBalanced()) {
                this->AvlBalance();
            }
            return res;
        }

        void Optimize() {
            this->AvlBalance();
        }

        void AvlUpdate() override {
            this->CleanUp();
            this->UpdateStats();
        }

        void AvlSwapContent(TaggedFragmentMapNode<T> &other) override {
            std::swap(this->content, other.content);
        }

        void Visit(std::function<void(const TaggedTextFragment<T> &)> &callback)
            const {
            if (this->begin) {
                this->begin->Visit(callback);
            }
            if (this->content) {
                callback(this->content.value());
            }
            if (this->end) {
                this->end->Visit(callback);
            }
        }

     private:
        void UpdateStats() {
            this->empty = (this->begin == nullptr || this->begin->Empty()) &&
                          !this->content.has_value() &&
                          (this->end == nullptr || this->end->Empty());
            std::size_t bheight =
                this->begin ? this->begin->GetHeight() + 1 : 0;
            std::size_t eheight = this->end ? this->end->GetHeight() + 1 : 0;
            this->height = std::max(bheight, eheight);
        }

        void CleanUp() {
            if (this->begin && this->begin->Empty()) {
                this->begin.reset();
            }
            if (this->end && this->end->Empty()) {
                this->end.reset();
            }
        }

        std::optional<TaggedTextFragment<T>> content;
        std::size_t height;
        bool empty;
    };

    template <typename T>
    class TaggedFragmentMap {
     public:
        TaggedFragmentMap() : root(nullptr) {}

        const TaggedTextFragment<T> *Get(const TextPosition &pos) {
            if (this->root) {
                return this->root->Get(pos);
            } else {
                return nullptr;
            }
        }

        template <typename I>
        void GetLine(TextPosition::Line line, I iter) {
            if (this->root) {
                this->root->GetLine(line, std::move(iter));
            }
        }

        const TaggedTextFragment<T> *Next(const TextPosition &pos) {
            if (this->root) {
                return this->root->Next(pos);
            } else {
                return nullptr;
            }
        }

        void Insert(const TextPosition &start, const TextPosition &length,
                    T &&tag) {
            TaggedTextFragment<T> fragment(start, length, std::forward<T>(tag));
            this->Insert(std::move(fragment));
        }

        void Insert(TaggedTextFragment<T> &&fragment) {
            if (this->root) {
                this->root->Insert(
                    std::forward<TaggedTextFragment<T>>(fragment));
            } else {
                this->root = std::make_unique<TaggedFragmentMapNode<T>>(
                    std::forward<TaggedTextFragment<T>>(fragment));
            }
        }

        std::optional<TextPosition> Remove(const TextPosition &pos) {
            std::optional<TextPosition> res;
            if (this->root) {
                res = this->root->Remove(pos);
                if (this->root->Empty()) {
                    this->root.reset();
                }
            }
            return res;
        }

        void Clear() {
            this->root.reset();
        }

        void Visit(
            std::function<void(const TaggedTextFragment<T> &)> callback) const {
            if (this->root) {
                this->root->Visit(callback);
            }
        }

     private:
        std::unique_ptr<TaggedFragmentMapNode<T>> root;
    };
}  // namespace sloked

#endif