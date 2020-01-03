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

#ifndef SLOKED_TEXT_SEARCH_REPLACE_H_
#define SLOKED_TEXT_SEARCH_REPLACE_H_

#include "sloked/core/Encoding.h"
#include "sloked/text/cursor/TransactionStream.h"
#include "sloked/text/cursor/TransactionCursor.h"
#include "sloked/text/cursor/TransactionBatch.h"
#include "sloked/text/cursor/Cursor.h"
#include "sloked/text/search/Entry.h"

namespace sloked {

    class SlokedTextReplacer {
     public:
        SlokedTextReplacer(TextBlock &, std::unique_ptr<SlokedTransactionStream>, const Encoding &);

        void Replace(const SlokedSearchEntry &, std::string_view, bool = true);

        template <typename T>
        void Replace(const T &begin, const T &end, std::string_view value, bool replace_groups = true) {
            TransactionBatch batch(*this->transactions, this->encoding);
            TransactionCursor cursor(this->text, this->encoding, batch);
            for (auto it = begin; it != end; ++it) {
                this->ReplaceImpl(cursor, *it, value, replace_groups);
            }
            batch.Finish();
        }

     private:
        void ReplaceImpl(SlokedCursor &, const SlokedSearchEntry &, std::string_view, bool);
        std::string Prepare(const SlokedSearchEntry &, std::string_view);

        TextBlock &text;
        std::unique_ptr<SlokedTransactionStream> transactions;
        const Encoding &encoding;
    };
}

#endif