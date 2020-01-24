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

#include "sloked/namespace/Path.h"
#include "sloked/core/Encoding.h"
#include "sloked/text/TextBlock.h"
#include "sloked/text/fragment/FragmentMap.h"
#include "sloked/core/Error.h"
#include "sloked/text/cursor/TransactionStream.h"
#include "sloked/core/Event.h"

namespace sloked {

    template <typename T>
    class SlokedTextTagger {
     public:
        using Unbind = std::function<void()>;
        virtual ~SlokedTextTagger() = default;
        virtual std::optional<TaggedTextFragment<T>> Get(const TextPosition &) = 0;
        virtual Unbind OnUpdate(std::function<void(const TextPosition &)>) = 0;
    };

    template <typename T>
    class SlokedTextTagIterator {
     public:
        using Unbind = std::function<void()>;
        virtual ~SlokedTextTagIterator() = default;
        virtual std::optional<TaggedTextFragment<T>> Next() = 0;
        virtual void Rewind(const TextPosition &) = 0;
        virtual const TextPosition &GetPosition() const = 0;
        virtual Unbind OnUpdate(std::function<void(const TextPosition &)>) = 0;
    };

    template <typename T>
    class SlokedTextProxyTagger : public SlokedTextTagger<T> {
     public:
        SlokedTextProxyTagger(std::unique_ptr<SlokedTextTagger<T>> tagger = nullptr)
            : tagger(std::move(tagger)), unsubscribe{nullptr} {
            if (this->tagger) {
                this->unsubscribe = this->tagger->OnUpdate([this](const auto &pos) {
                    this->subscribers.Emit(pos);
                });
            }
        }

        ~SlokedTextProxyTagger() {
            if (this->unsubscribe) {
                this->unsubscribe();
            }
        }

        void ChangeTagger(std::unique_ptr<SlokedTextTagger<T>> tagger = nullptr) {
            if (this->unsubscribe) {
                this->unsubscribe();
                this->unsubscribe = nullptr;
            }
            this->tagger = std::move(tagger);
            if (this->tagger) {
                this->unsubscribe = this->tagger->OnUpdate([this](const auto &pos) {
                    this->subscribers.Emit(pos);
                });
            }
        }

        bool HasTagger() const {
            return this->tagger != nullptr;
        }

        std::optional<TaggedTextFragment<T>> Get(const TextPosition &pos) final {
            if (this->tagger) {
                return this->tagger->Get(pos);
            } else {
                return {};
            }
        }

        typename SlokedTextTagger<T>::Unbind OnUpdate(std::function<void(const TextPosition &)> subscriber) final {
            return this->subscribers.Listen(std::move(subscriber));
        }

     private:
        std::unique_ptr<SlokedTextTagger<T>> tagger;
        typename SlokedTextTagger<T>::Unbind unsubscribe;
        SlokedEventEmitter<const TextPosition &> subscribers;
    };

    class SlokedTaggableDocument {
     public:
        virtual ~SlokedTaggableDocument() = default;
        virtual bool HasUpstream() const = 0;
        virtual std::optional<SlokedPath> GetUpstream() const = 0;
        virtual std::optional<std::string> GetUpstreamURI() const = 0;
        virtual const TextBlockView &GetText() const = 0;
        virtual const Encoding &GetEncoding() const = 0;
        virtual SlokedTransactionListenerManager &GetTransactionListeners() = 0;
    };

    template <typename T>
    class SlokedTextTaggerFactory {
     public:
        virtual ~SlokedTextTaggerFactory() = default;
        virtual std::unique_ptr<SlokedTextTagger<T>> Create(SlokedTaggableDocument &) const = 0;
    };

    template <typename T>
    class SlokedTextTaggerRegistry {
     public:
        virtual ~SlokedTextTaggerRegistry() = default;
        virtual bool Has(const std::string &id) const = 0;
        virtual std::unique_ptr<SlokedTextTagger<T>> Create(const std::string &, SlokedTaggableDocument &) const = 0;
        virtual std::unique_ptr<SlokedTextTagger<T>> TryCreate(const std::string &, SlokedTaggableDocument &) const = 0;
    };

    template <typename T>
    class SlokedDefaultTextTaggerRegistry : public SlokedTextTaggerRegistry<T> {
     public:
        SlokedDefaultTextTaggerRegistry(SlokedTextTaggerRegistry<T> *base = nullptr)
            : base(base) {}

        bool Has(const std::string &id) const final {
            return this->factories.count(id) != 0 ||    
                (this->base != nullptr && this->base->Has(id));
        }

        std::unique_ptr<SlokedTextTagger<T>> Create(const std::string &id, SlokedTaggableDocument &document) const final {
            if (this->factories.count(id) != 0) {
                return this->factories.at(id)->Create(document);
            } else if (this->base != nullptr) {
                return this->base->Create(id, document);
            } else {
                throw SlokedError("TextTaggerRegistry: Unknown tagger \'" + id + "\'");
            }
        }

        std::unique_ptr<SlokedTextTagger<T>> TryCreate(const std::string &id, SlokedTaggableDocument &document) const final {
            if (this->Has(id)) {
                return this->Create(id, document);
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
    class SlokedLazyTaggedText : public SlokedTextTagger<T> {
     public:
        SlokedLazyTaggedText(SlokedTextTagIterator<T> &tagger)
            : tagger(tagger), current{0, 0} {
            this->unsubscribe = this->tagger.OnUpdate([this](const auto &pos) {
                this->fragments.Remove(pos);
                this->current = std::min(pos, this->current);
                this->emitter.Emit(pos);
            });
            this->NextFragment();
        }

        ~SlokedLazyTaggedText() {
            if (this->unsubscribe) {
                this->unsubscribe();
            }
        }

        std::optional<TaggedTextFragment<T>> Get(const TextPosition &pos) final {
            while (!(pos < this->current)) {
                this->NextFragment();
            }
            auto fragment = this->fragments.Get(pos);
            if (fragment) {
                return *fragment;
            } else {
                return {};
            }
        }

        typename SlokedTextTagger<T>::Unbind OnUpdate(std::function<void(const TextPosition &)> callback) final {
            return this->emitter.Listen(std::move(callback));
        }
    
     private:
        void NextFragment() {
            auto fragment = this->tagger.Next();
            if (fragment.has_value()) {
                this->current = fragment.value().GetEnd();
                this->fragments.Insert(std::move(fragment.value()));
            } else {
                this->current = this->tagger.GetPosition();
            }
        }

        SlokedTextTagIterator<T> &tagger;
        TaggedFragmentMap<T> fragments;
        TextPosition current;
        typename SlokedTextTagger<T>::Unbind unsubscribe;
        SlokedEventEmitter<const TextPosition &> emitter;
    };

    template <typename T>
    class SlokedCacheTaggedText : public SlokedTextTagger<T> {
     public:
        SlokedCacheTaggedText(SlokedTextTagger<T> &tags)
            : tags(tags), fragment{} {
            this->unsubscribe = this->tags.OnUpdate([this](const auto &pos) {
                this->fragment = {};
                emitter.Emit(pos);
            });
        }
        
        ~SlokedCacheTaggedText() {
            if (this->unsubscribe) {
                this->unsubscribe();
            }
        }

        std::optional<TaggedTextFragment<T>> Get(const TextPosition &position) final {
            if (!this->fragment.has_value() || !this->fragment.value().Includes(position)) {
                this->fragment = this->tags.Get(position);
            }
            return this->fragment;
        }

        typename SlokedTextTagger<T>::Unbind OnUpdate(std::function<void(const TextPosition &)> callback) final {
            return this->emitter.Listen(std::move(callback));
        }

     private:
        SlokedTextTagger<T> &tags;
        std::optional<TaggedTextFragment<T>> fragment;
        typename SlokedTextTagger<T>::Unbind unsubscribe;
        SlokedEventEmitter<const TextPosition &> emitter;
    };

    template <typename T>
    class SlokedTaggedTextView : public SlokedTextTagger<T> {
     public:
        SlokedTaggedTextView(SlokedTextTagger<T> &tags, const TextPosition &offset, const TextPosition &size)
            : tags(tags), offset(offset) {
            this->end = offset + size;
        }

        std::optional<TaggedTextFragment<T>> Get(const TextPosition &position) final {
            const TaggedTextFragment<T> *fragment = this->mapped.Get(position);
            if (fragment != nullptr) {
                return *fragment;
            }
            TextPosition realPosition = position + this->offset;
            if (!(realPosition < this->end)) {
                return {};
            }
            auto realFragment = this->tags.Get(realPosition);
            if (realFragment) {
                const auto &realStart = realFragment->GetStart();
                const auto &realEnd = realFragment->GetEnd();
                TextPosition fragmentStart{std::max(realStart.line, this->offset.line) - this->offset.line, std::max(realStart.column, this->offset.column) - this->offset.column};
                TextPosition fragmentEnd{std::min(realEnd.line, this->end.line) - this->offset.line, std::min(realEnd.column, this->end.column) - this->offset.column};
                this->mapped.Insert(fragmentStart, fragmentEnd - fragmentStart, T{realFragment->GetTag()});
                fragment = this->mapped.Get(position);
                if (fragment != nullptr) {
                    return *fragment;
                }
            }
            return {};
        }

        typename SlokedTextTagger<T>::Unbind OnUpdate(std::function<void(const TextPosition &)> callback) final {
            return this->tags.OnUpdate(std::move(callback));
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
        SlokedTextTagger<T> &tags;
        TaggedFragmentMap<T> mapped;
        TextPosition offset;
        TextPosition end;
    };
}

#endif