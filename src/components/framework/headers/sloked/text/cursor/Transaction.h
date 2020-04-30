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

#ifndef SLOKED_TEXT_CURSOR_TRANSACTION_H_
#define SLOKED_TEXT_CURSOR_TRANSACTION_H_

#include <string>
#include <variant>
#include <vector>

#include "sloked/Base.h"
#include "sloked/core/Encoding.h"
#include "sloked/core/Position.h"
#include "sloked/core/RangeMap.h"
#include "sloked/text/TextBlock.h"

namespace sloked {

    class SlokedTransactionPatch {
     public:
        SlokedTransactionPatch();
        void NextTransaction();
        void Insert(const TextPosition &, const TextPosition &,
                    const TextPositionDelta &);
        bool Has(const TextPosition &) const;
        TextPositionDelta At(const TextPosition &) const;

     private:
        std::vector<RangeMap<TextPosition, TextPositionDelta>> patch;
    };

    class SlokedCursorTransaction {
     public:
        enum class Action {
            Insert,
            Newline,
            DeleteForward,
            DeleteBackward,
            Clear,
            Batch
        };

        struct DeletePosition {
            TextPosition position;
            std::string content;
            TextPosition::Column width;
        };

        struct Content {
            TextPosition position;
            std::string content;
        };

        struct Range {
            TextPosition from;
            TextPosition to;
            std::vector<std::string> content;
        };

        using Batch = std::vector<SlokedCursorTransaction>;

        SlokedCursorTransaction(Action, const Content &);
        SlokedCursorTransaction(Action, const DeletePosition &);
        SlokedCursorTransaction(const Range &);
        SlokedCursorTransaction(const Batch &);

        TextPosition GetPosition() const;
        void Commit(TextBlock &, const Encoding &) const;
        void Rollback(TextBlock &, const Encoding &) const;
        SlokedTransactionPatch CommitPatch(const Encoding &) const;
        SlokedTransactionPatch RollbackPatch(const Encoding &) const;
        void Apply(const SlokedTransactionPatch &);
        void Update(TextBlock &, const Encoding &);

     private:
        void CommitPatch(const Encoding &, SlokedTransactionPatch &) const;
        void RollbackPatch(const Encoding &, SlokedTransactionPatch &) const;

        Action action;
        std::variant<DeletePosition, Content, Range, Batch> argument;
    };
}  // namespace sloked

#endif
