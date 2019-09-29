/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

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

#ifndef SLOKED_TEXT_FRAGMENT_UPDATER_H_
#define SLOKED_TEXT_FRAGMENT_UPDATER_H_

#include "sloked/text/cursor/TransactionStream.h"
#include "sloked/text/fragment/TaggedText.h"

namespace sloked {

    template <typename T>
    class SlokedFragmentUpdater : public SlokedTransactionStream::Listener {
    public:
        SlokedFragmentUpdater(const TextBlockView &text, SlokedTaggedText<T> &tags, const Encoding &encoding, const SlokedCharWidth &charWidth)
            : text(text), tags(tags), encoding(encoding), charWidth(charWidth) {}

        void OnCommit(const SlokedCursorTransaction &trans) override {
            auto realPos = trans.GetPosition();
            TextPosition pos{realPos.line > 0 ? realPos.line - 1 : 0 , 0};
            this->tags.Rewind(pos);
        }

        void OnRollback(const SlokedCursorTransaction &trans) override {
            auto realPos = trans.GetPosition();
            TextPosition pos{realPos.line > 0 ? realPos.line - 1 : 0 , 0};
            this->tags.Rewind(pos);
        }

        void OnRevert(const SlokedCursorTransaction &trans) override {
            auto realPos = trans.GetPosition();
            TextPosition pos{realPos.line > 0 ? realPos.line - 1 : 0 , 0};
            this->tags.Rewind(pos);
        }

    private:
        const TextBlockView &text;
        SlokedTaggedText<T> &tags;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
    };
}

#endif