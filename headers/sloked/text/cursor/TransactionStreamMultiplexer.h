/*
  SPDX-License-Identifier: LGPL-3.0-or-later

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONSTREAMMULTIPLEXER_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONSTREAMMULTIPLEXER_H_

#include "sloked/text/cursor/TransactionStream.h"
#include <vector>
#include <map>
#include <utility>

namespace sloked {

    class TransactionStreamMultiplexer {
     public:
        TransactionStreamMultiplexer(TextBlock &, const Encoding &);
        std::unique_ptr<SlokedTransactionStream> NewStream();

        class Stream;
        friend class Stream;

     private:
        using StreamId = std::size_t;
        struct LabeledTransaction {
            StreamId stream;
            std::size_t stamp;
            SlokedCursorTransaction transaction;
        };

        StreamId RegisterStream(SlokedTransactionStream::Listener &);
        void RemoveStream(StreamId);
        TextPosition Commit(StreamId, const SlokedCursorTransaction &);
        bool HasRollback(StreamId) const;
        TextPosition Rollback(StreamId);
        bool HasRevertable(StreamId) const;
        TextPosition RevertRollback(StreamId);
        void TriggerListeners(void (SlokedTransactionStream::Listener::*)(const SlokedCursorTransaction &), const SlokedCursorTransaction &);

        TextBlock &text;
        const Encoding &encoding;
        StreamId nextStreamId;
        std::size_t nextTransactionStamp;
        std::vector<LabeledTransaction> journal;
        std::map<std::size_t, std::vector<LabeledTransaction>> backtrack;
        std::map<StreamId, std::reference_wrapper<SlokedTransactionStream::Listener>> listeners;
    };
}

#endif