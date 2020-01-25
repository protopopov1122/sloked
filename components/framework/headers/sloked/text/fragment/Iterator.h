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

#ifndef SLOKED_TEXT_FRAGMENT_ITERATOR_H_
#define SLOKED_TEXT_FRAGMENT_ITERATOR_H_

#include "sloked/text/fragment/TaggedText.h"

namespace sloked {

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
}

#endif