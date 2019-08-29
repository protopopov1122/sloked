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

#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONSTREAM_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONSTREAM_H_

#include "sloked/text/cursor/Transaction.h"
#include <memory>

namespace sloked {

    class SlokedTransactionStream {
     public:
        class Listener {
         public:
            virtual ~Listener() = default;
            virtual void OnCommit(const SlokedCursorTransaction &) = 0;
            virtual void OnRollback(const SlokedCursorTransaction &) = 0;
            virtual void OnRevert(const SlokedCursorTransaction &) = 0;
        };

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
}

#endif