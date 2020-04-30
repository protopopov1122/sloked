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

#ifndef SLOKED_TEXT_FRAGMENT_TAGGEDTEXT_H_
#define SLOKED_TEXT_FRAGMENT_TAGGEDTEXT_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/Error.h"
#include "sloked/core/Event.h"
#include "sloked/namespace/Path.h"
#include "sloked/text/TextBlock.h"
#include "sloked/text/cursor/TransactionStream.h"
#include "sloked/text/fragment/FragmentMap.h"

namespace sloked {

    template <typename T>
    class SlokedTextTagger {
     public:
        using ChangeListener = std::function<void(const TextPositionRange &)>;
        using Unbind = std::function<void()>;
        virtual ~SlokedTextTagger() = default;
        virtual std::optional<TaggedTextFragment<T>> Get(
            const TextPosition &) = 0;
        virtual std::vector<TaggedTextFragment<T>> Get(TextPosition::Line) = 0;
        virtual Unbind OnChange(ChangeListener) = 0;
    };

    template <typename T>
    class SlokedTextProxyTagger : public SlokedTextTagger<T> {
     public:
        SlokedTextProxyTagger(
            std::unique_ptr<SlokedTextTagger<T>> tagger = nullptr)
            : tagger(std::move(tagger)), unsubscribe{nullptr} {
            if (this->tagger) {
                this->unsubscribe = this->tagger->OnChange(
                    [this](const auto &pos) { this->subscribers.Emit(pos); });
            }
        }

        ~SlokedTextProxyTagger() {
            if (this->unsubscribe) {
                this->unsubscribe();
            }
        }

        void ChangeTagger(
            std::unique_ptr<SlokedTextTagger<T>> tagger = nullptr) {
            if (this->unsubscribe) {
                this->unsubscribe();
                this->unsubscribe = nullptr;
            }
            this->tagger = std::move(tagger);
            if (this->tagger) {
                this->unsubscribe = this->tagger->OnChange(
                    [this](const auto &pos) { this->subscribers.Emit(pos); });
            }
        }

        bool HasTagger() const {
            return this->tagger != nullptr;
        }

        std::optional<TaggedTextFragment<T>> Get(
            const TextPosition &pos) final {
            if (this->tagger) {
                return this->tagger->Get(pos);
            } else {
                return {};
            }
        }

        std::vector<TaggedTextFragment<T>> Get(TextPosition::Line line) final {
            if (this->tagger) {
                return this->tagger->Get(line);
            } else {
                return {};
            }
        }

        typename SlokedTextTagger<T>::Unbind OnChange(
            typename SlokedTextTagger<T>::ChangeListener subscriber) final {
            return this->subscribers.Listen(std::move(subscriber));
        }

     private:
        std::unique_ptr<SlokedTextTagger<T>> tagger;
        typename SlokedTextTagger<T>::Unbind unsubscribe;
        SlokedEventEmitter<const TextPositionRange &> subscribers;
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
        virtual std::unique_ptr<SlokedTextTagger<T>> Create(
            SlokedTaggableDocument &) const = 0;
    };

    template <typename T>
    class SlokedTextTaggerRegistry {
     public:
        virtual ~SlokedTextTaggerRegistry() = default;
        virtual bool Has(const std::string &id) const = 0;
        virtual std::unique_ptr<SlokedTextTagger<T>> Create(
            const std::string &, SlokedTaggableDocument &) const = 0;
        virtual std::unique_ptr<SlokedTextTagger<T>> TryCreate(
            const std::string &, SlokedTaggableDocument &) const = 0;
    };

    template <typename T>
    class SlokedDefaultTextTaggerRegistry : public SlokedTextTaggerRegistry<T> {
     public:
        SlokedDefaultTextTaggerRegistry(
            SlokedTextTaggerRegistry<T> *base = nullptr)
            : base(base) {}

        bool Has(const std::string &id) const final {
            return this->factories.count(id) != 0 ||
                   (this->base != nullptr && this->base->Has(id));
        }

        std::unique_ptr<SlokedTextTagger<T>> Create(
            const std::string &id,
            SlokedTaggableDocument &document) const final {
            if (this->factories.count(id) != 0) {
                return this->factories.at(id)->Create(document);
            } else if (this->base != nullptr) {
                return this->base->Create(id, document);
            } else {
                throw SlokedError("TextTaggerRegistry: Unknown tagger \'" + id +
                                  "\'");
            }
        }

        std::unique_ptr<SlokedTextTagger<T>> TryCreate(
            const std::string &id,
            SlokedTaggableDocument &document) const final {
            if (this->Has(id)) {
                return this->Create(id, document);
            } else {
                return nullptr;
            }
        }

        void Bind(const std::string &id,
                  std::unique_ptr<SlokedTextTaggerFactory<T>> factory) {
            this->factories.emplace(id, std::move(factory));
        }

     private:
        SlokedTextTaggerRegistry<T> *base;
        std::map<std::string, std::unique_ptr<SlokedTextTaggerFactory<T>>>
            factories;
    };

    template <typename T>
    class SlokedCacheTaggedText : public SlokedTextTagger<T> {
     public:
        SlokedCacheTaggedText(SlokedTextTagger<T> &tags)
            : tags(tags), fragment{} {
            this->unsubscribe = this->tags.OnChange([this](const auto &pos) {
                this->fragment = {};
                emitter.Emit(pos);
            });
        }

        ~SlokedCacheTaggedText() {
            if (this->unsubscribe) {
                this->unsubscribe();
            }
        }

        std::optional<TaggedTextFragment<T>> Get(
            const TextPosition &position) final {
            if (!this->fragment.has_value() ||
                !this->fragment.value().Includes(position)) {
                this->fragment = this->tags.Get(position);
            }
            return this->fragment;
        }

        std::vector<TaggedTextFragment<T>> Get(TextPosition::Line line) final {
            return this->tags.Get(line);
        }

        typename SlokedTextTagger<T>::Unbind OnChange(
            std::function<void(const TextPositionRange &)> callback) final {
            return this->emitter.Listen(std::move(callback));
        }

     private:
        SlokedTextTagger<T> &tags;
        std::optional<TaggedTextFragment<T>> fragment;
        typename SlokedTextTagger<T>::Unbind unsubscribe;
        SlokedEventEmitter<const TextPositionRange &> emitter;
    };
}  // namespace sloked

#endif