#ifndef SLOKED_TEXT_FRAGMENT_FRAGMENTMAP_H_
#define SLOKED_TEXT_FRAGMENT_FRAGMENTMAP_H_

#include "sloked/core/AVL.h"
#include "sloked/core/Error.h"
#include "sloked/text/fragment/TextFragment.h"
#include <algorithm>
#include <optional>

namespace sloked {

    template <typename T>
    class TaggedFragmentMapNode : public AVLNode<TaggedFragmentMapNode<T>> {
     public:
        TaggedFragmentMapNode(TaggedTextFragment<T> &&fragment)
            : AVLNode<TaggedFragmentMapNode<T>>(nullptr, nullptr), content(std::forward<TaggedTextFragment<T>>(fragment)), height(0), empty(false) {}

        std::size_t GetHeight() const {
            return this->height;
        }

        bool Empty() const {
            return this->empty;
        }

        bool Has(const TextPosition &pos) {
            return (this->content.has_value() && this->content.value().Includes(pos)) ||
                (this->begin && this->begin->Has(pos)) ||
                (this->end && this->end->Has(pos));
        }

        const TaggedTextFragment<T> *Min() const {
            const TaggedTextFragment<T> *res = this->content.has_value() ? &this->content.value() : nullptr;
            if (this->begin) {
                const TaggedTextFragment<T> *bres = this->begin->Min();
                if (bres) {
                    res = bres;
                }
            }
            return res;
        }

        const TaggedTextFragment<T> *Max() const {
            const TaggedTextFragment<T> *res = this->content.has_value() ? &this->content.value() : nullptr;
            if (this->end) {
                const TaggedTextFragment<T> *eres = this->begin->Max();
                if (eres) {
                    res = eres;
                }
            }
            return res;
        }

        const TaggedTextFragment<T> *Get(const TextPosition &pos) {
            const TaggedTextFragment<T> *fragment = nullptr;
            if (this->content.has_value() && this->content.value().Includes(pos)) {
                fragment = &this->content.value();
            }
            if (fragment == nullptr && this->begin) {
                fragment = this->begin->Get(pos);
            }
            if (fragment == nullptr && this->end) {
                fragment = this->end->Get(pos);
            }
            return fragment;
        }

        void Insert(TaggedTextFragment<T> &&fragment) {
            if (this->content.has_value()) {
                if (this->content.value().Overlaps(fragment)) {
                    throw SlokedError("Given fragment overlaps existing");
                }
                if (fragment < this->content) {
                    if (this->begin) {
                        this->begin->Insert(std::forward<TaggedTextFragment<T>>(fragment));
                    } else {
                        this->begin = std::make_unique<TaggedFragmentMapNode<T>>(std::forward<TaggedTextFragment<T>>(fragment));
                    }
                } else {
                    if (this->end) {
                        this->end->Insert(std::forward<TaggedTextFragment<T>>(fragment));
                    } else {
                        this->end = std::make_unique<TaggedFragmentMapNode<T>>(std::forward<TaggedTextFragment<T>>(fragment));
                    }
                }
            } else {
                auto max_begin = this->begin ? this->begin->Max() : nullptr;
                auto min_end = this->end ? this->end->Min() : nullptr;
                if (max_begin && fragment < *max_begin) {
                    this->begin->Insert(std::forward<TaggedTextFragment<T>>(fragment));
                } else if (min_end && *min_end < fragment) {
                    this->end->Insert(std::forward<TaggedTextFragment<T>>(fragment));
                } else {
                    this->content = std::forward<TaggedTextFragment<T>>(fragment);
                }
            }
            this->UpdateStats();
        }

        void Remove(const TextPosition &pos) {
            if (this->content.has_value()) {
                auto start = this->content.value().GetStart();
                auto len = this->content.value().GetLength();
                TextPosition end{start.line + len.line, start.column + len.column};
                if (pos < start || pos == start || (start < pos && pos < end)) {
                    this->content.reset();
                }
            }
            if (this->begin) {
                this->begin->Remove(pos);
            }
            if (this->end) {
                this->end->Remove(pos);
            }
            this->UpdateStats();
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

     private:
        void UpdateStats() {
            this->empty = (this->begin == nullptr || this->begin->Empty()) &&
                !this->content.has_value() &&
                (this->end == nullptr || this->end->Empty());
            std::size_t bheight = this->begin ? this->begin->GetHeight() + 1 : 0;
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
        TaggedFragmentMap()
            : root(nullptr) {}
    
        const TaggedTextFragment<T> *Get(const TextPosition &pos) {
            if (this->root) {
                return this->root->Get(pos);
            } else {
                return nullptr;
            }
        }

        void Insert(const TextPosition &start, const TextPosition &length, T &&tag) {
            TaggedTextFragment<T> fragment(start, length, std::forward<T>(tag));
            if (this->root) {
                this->root->Insert(std::move(fragment));
                this->root->Optimize();
            } else {
                this->root = std::make_unique<TaggedFragmentMapNode<T>>(std::move(fragment));
            }
        }

        void Remove(const TextPosition &pos) {
            if (this->root) {
                this->root->Remove(pos);
                this->root->Optimize();
            }
            if (this->root->Empty()) {
                this->root.reset();
            }
        }

        void Clear() {
            this->root.reset();
        }

     private:
        std::unique_ptr<TaggedFragmentMapNode<T>> root;
    };
}

#endif