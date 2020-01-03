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

#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONJOURNAL_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONJOURNAL_H_

#include "sloked/Base.h"

namespace sloked {

    class SlokedTransactionJournal {
     public:
        virtual ~SlokedTransactionJournal() = default;
        virtual void Undo() = 0;
        virtual bool HasUndoable() const = 0;
        virtual void Redo() = 0;
        virtual bool HasRedoable() const = 0;
    };
}

#endif