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

#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONSTREAM_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONSTREAM_H_

#include <memory>

#include "sloked/text/cursor/Transaction.h"

namespace sloked {

    class SlokedTransactionStreamListener {
     public:
        virtual ~SlokedTransactionStreamListener() = default;
        virtual void OnCommit(const SlokedCursorTransaction &) = 0;
        virtual void OnRollback(const SlokedCursorTransaction &) = 0;
        virtual void OnRevert(const SlokedCursorTransaction &) = 0;
    };

    class SlokedTransactionListenerManager {
     public:
        virtual ~SlokedTransactionListenerManager() = default;
        virtual void AddListener(
            std::shared_ptr<SlokedTransactionStreamListener>) = 0;
        virtual void RemoveListener(
            const SlokedTransactionStreamListener &) = 0;
        virtual void ClearListeners() = 0;
    };

    class SlokedTransactionStream : public SlokedTransactionListenerManager {
     public:
        using Listener = SlokedTransactionStreamListener;
        virtual ~SlokedTransactionStream() = default;
        virtual TextPosition Commit(const SlokedCursorTransaction &) = 0;
        virtual bool HasRollback() const = 0;
        virtual TextPosition Rollback() = 0;
        virtual bool HasRevertable() const = 0;
        virtual TextPosition RevertRollback() = 0;

        virtual void AddListener(std::shared_ptr<Listener>) = 0;
        virtual void RemoveListener(const Listener &) = 0;
        virtual void ClearListeners() = 0;
    };
}  // namespace sloked

#endif