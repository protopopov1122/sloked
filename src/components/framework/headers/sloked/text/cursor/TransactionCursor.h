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

#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONCURSOR_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONCURSOR_H_

#include <memory>
#include <vector>

#include "sloked/core/Encoding.h"
#include "sloked/text/TextBlock.h"
#include "sloked/text/cursor/Cursor.h"
#include "sloked/text/cursor/Transaction.h"
#include "sloked/text/cursor/TransactionJournal.h"
#include "sloked/text/cursor/TransactionStream.h"

namespace sloked {

    class TransactionCursor : public SlokedCursor,
                              public SlokedTransactionJournal {
     public:
        TransactionCursor(TextBlock &, const Encoding &,
                          SlokedTransactionStream &);
        virtual ~TransactionCursor();

        void Undo() override;
        bool HasUndoable() const override;
        void Redo() override;
        bool HasRedoable() const override;

        Line GetLine() const override;
        Column GetColumn() const override;

        void SetPosition(Line, Column) override;
        void MoveUp(Line) override;
        void MoveDown(Line) override;
        void MoveForward(Column) override;
        void MoveBackward(Column) override;

        void Insert(std::string_view) override;
        void NewLine(std::string_view) override;
        void DeleteBackward() override;
        void DeleteForward() override;
        using SlokedCursor::ClearRegion;
        void ClearRegion(const TextPosition &, const TextPosition &) override;

     private:
        void applyCommand(const SlokedCursorTransaction &);

        TextBlock &text;
        const Encoding &encoding;
        SlokedTransactionStream &stream;
        Line line;
        Column column;
        std::shared_ptr<SlokedTransactionStream::Listener> listener;
        std::vector<TextPositionDelta> cursors;
    };
}  // namespace sloked

#endif