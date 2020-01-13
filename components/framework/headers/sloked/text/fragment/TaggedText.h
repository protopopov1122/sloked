/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_TEXT_FRAGMENT_TAGGEDTEXT_H_
#define SLOKED_TEXT_FRAGMENT_TAGGEDTEXT_H_

#include "sloked/core/Encoding.h"
#include "sloked/text/TextBlock.h"
#include "sloked/text/fragment/FragmentMap.h"
#include "sloked/core/Error.h"

namespace sloked {

    template <typename T>
    class SlokedTextTagger {
     public:
        virtual ~SlokedTextTagger() = default;
        virtual std::optional<TaggedTextFragment<T>> Next() = 0;
        virtual void Rewind(const TextPosition &) = 0;
        virtual const TextPosition &GetPosition() const = 0;
    };

    template <typename T>
    class SlokedTextTaggerFactory {
     public:
        virtual ~SlokedTextTaggerFactory() = default;
        virtual std::unique_ptr<SlokedTextTagger<T>> Create(const TextBlockView &, const Encoding &, const SlokedCharWidth &) const = 0;
    };

    template <typename T>
    class SlokedTextTaggerRegistry {
     public:
        virtual ~SlokedTextTaggerRegistry() = default;
        virtual bool Has(const std::string &id) const = 0;
        virtual std::unique_ptr<SlokedTextTagger<T>> Create(const std::string &, const TextBlockView &, const Encoding &, const SlokedCharWidth &) const = 0;
        virtual std::unique_ptr<SlokedTextTagger<T>> TryCreate(const std::string &, const TextBlockView &, const Encoding &, const SlokedCharWidth &) const = 0;
        virtual void Bind(const std::string &, std::unique_ptr<SlokedTextTaggerFactory<T>>) = 0;
    };

    template <typename T>
    class SlokedDefaultTextTaggerRegistry : public SlokedTextTaggerRegistry<T> {
     public:
        SlokedDefaultTextTaggerRegistry(SlokedTextTaggerRegistry<T> *base = nullptr)
            : base(base) {}

        bool Has(const std::string &id) const {
            return this->factories.count(id) != 0 ||    
                (this->base != nullptr && this->base->Has(id));
        }

        std::unique_ptr<SlokedTextTagger<T>> Create(const std::string &id, const TextBlockView &text, const Encoding &encoding, const SlokedCharWidth &charWidth) const {
            if (this->factories.count(id) != 0) {
                return this->factories.at(id)->Create(text, encoding, charWidth);
            } else if (this->base != nullptr) {
                return this->base->Create(id, text, encoding, charWidth);
            } else {
                throw SlokedError("TextTaggerRegistry: Unknown tagger \'" + id + "\'");
            }
        }

        std::unique_ptr<SlokedTextTagger<T>> TryCreate(const std::string &id, const TextBlockView &text, const Encoding &encoding, const SlokedCharWidth &charWidth) const {
            if (this->Has(id)) {
                return this->Create(id, text, encoding, charWidth);
            }  else {
                return nullptr;
            }
        }

        void Bind(const std::string &id, std::unique_ptr<SlokedTextTaggerFactory<T>> factory) {
            this->factories.emplace(id, std::move(factory));
        }

     private:
        SlokedTextTaggerRegistry<T> *base;
        std::map<std::string, std::unique_ptr<SlokedTextTaggerFactory<T>>> factories;
    };

    template <typename T>
    class SlokedTaggedText {
     public:
        virtual ~SlokedTaggedText() = default;
        virtual const TaggedTextFragment<T> *Get(const TextPosition &) = 0;
        virtual void Rewind(const TextPosition &) = 0;
    };

    template <typename T>
    class SlokedLazyTaggedText : public SlokedTaggedText<T> {
     public:
        SlokedLazyTaggedText(std::unique_ptr<SlokedTextTagger<T>> tagger)
            : tagger(std::move(tagger)), current{0, 0} {
            this->NextFragment();
        }

        const TaggedTextFragment<T> *Get(const TextPosition &pos) override {
            while (!(pos < this->current)) {
                this->NextFragment();
            }
            return this->fragments.Get(pos);
        }

        void Rewind(const TextPosition &pos) override {
            this->fragments.Remove(pos);
            if (this->tagger) {
                this->tagger->Rewind(pos);
            }
            this->current = std::min(this->current, pos);
        }
    
     private:
        void NextFragment() {
            if (this->tagger) {
                auto fragment = this->tagger->Next();
                if (fragment.has_value()) {
                    this->current = fragment.value().GetEnd();
                    this->fragments.Insert(std::move(fragment.value()));
                } else {
                    this->current = this->tagger->GetPosition();
                }
            } else {
                this->current = TextPosition::Max;
            }
        }

        std::unique_ptr<SlokedTextTagger<T>> tagger;
        TaggedFragmentMap<T> fragments;
        TextPosition current;
    };

    template <typename T>
    class SlokedCacheTaggedText : public SlokedTaggedText<T> {
     public:
        SlokedCacheTaggedText(SlokedTaggedText<T> &tags)
            : tags(tags), fragment(nullptr) {}

        const TaggedTextFragment<T> *Get(const TextPosition &position) override {
            if (this->fragment == nullptr || !this->fragment->Includes(position)) {
                this->fragment = this->tags.Get(position);
            }
            return this->fragment;
        }

        void Rewind(const TextPosition &position) override {
            this->tags.Rewind(position);
            this->fragment = nullptr;
        }

     private:
        SlokedTaggedText<T> &tags;
        const TaggedTextFragment<T> *fragment;
    };

    template <typename T>
    class SlokedTaggedTextView : public SlokedTaggedText<T> {
     public:
        SlokedTaggedTextView(SlokedTaggedText<T> &tags, const TextPosition &offset, const TextPosition &size)
            : tags(tags), offset(offset) {
            this->end = offset + size;
        }

        const TaggedTextFragment<T> *Get(const TextPosition &position) override {
            const TaggedTextFragment<T> *fragment = this->mapped.Get(position);
            if (fragment != nullptr) {
                return fragment;
            }
            TextPosition realPosition = position + this->offset;
            if (!(realPosition < this->end)) {
                return nullptr;
            }
            auto realFragment = this->tags.Get(realPosition);
            if (realFragment) {
                const auto &realStart = realFragment->GetStart();
                const auto &realEnd = realFragment->GetEnd();
                TextPosition fragmentStart{std::max(realStart.line, this->offset.line) - this->offset.line, std::max(realStart.column, this->offset.column) - this->offset.column};
                TextPosition fragmentEnd{std::min(realEnd.line, this->end.line) - this->offset.line, std::min(realEnd.column, this->end.column) - this->offset.column};
                this->mapped.Insert(fragmentStart, fragmentEnd - fragmentStart, T{realFragment->GetTag()});
                return this->mapped.Get(position);
            } else {
                return nullptr;
            }
        }

        void Rewind(const TextPosition &position) override {
            TextPosition realPosition = position + this->offset;
            if (realPosition < this->end) {
                this->tags.Rewind(realPosition);
                this->mapped.Remove(this->mapped.NearestLeast(position).value_or(TextPosition{0, 0}));
            }
        }

        void Update(const TextPosition &offset, const TextPosition &size) {
            this->offset = offset;
            this->end = offset + size;
            this->mapped.Clear();
        }

        void Reset() {
            this->mapped.Clear();
        }

     private:
        SlokedTaggedText<T> &tags;
        TaggedFragmentMap<T> mapped;
        TextPosition offset;
        TextPosition end;
    };
}

#endif