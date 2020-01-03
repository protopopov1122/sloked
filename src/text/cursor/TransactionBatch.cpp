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

#include "sloked/text/cursor/TransactionBatch.h"

namespace sloked {

    TransactionBatch::TransactionBatch(SlokedTransactionStream &stream, const Encoding &encoding)
        : stream(stream), encoding(encoding) {}

    TextPosition TransactionBatch::Commit(const SlokedCursorTransaction &trans) {
        this->rollback.clear();
        this->batch.push_back(trans);
        this->TriggerListeners(&SlokedTransactionStream::Listener::OnCommit, trans);
        auto pos = trans.GetPosition();
        auto patch = trans.CommitPatch(this->encoding);
        if (patch.Has(pos)) {
            const auto &delta = patch.At(pos);
            pos.line += delta.line;
            pos.column += delta.column;
        }
        return pos;
    }

    bool TransactionBatch::HasRollback() const {
        return !this->batch.empty();
    }

    TextPosition TransactionBatch::Rollback() {
        if (!this->batch.empty()) {
            auto trans = this->batch.back();
            this->rollback.push_back(trans);
            this->batch.pop_back();
            this->TriggerListeners(&SlokedTransactionStream::Listener::OnRollback, trans);
            return trans.GetPosition();
        }
        return TextPosition{};
    }

    bool TransactionBatch::HasRevertable() const {
        return !this->rollback.empty();
    }

    TextPosition TransactionBatch::RevertRollback() {
        if (!this->rollback.empty()) {
            auto trans = this->rollback.back();
            this->batch.push_back(trans);
            this->rollback.pop_back();
            this->TriggerListeners(&SlokedTransactionStream::Listener::OnRevert, trans);
            auto patch = trans.CommitPatch(this->encoding);
            auto pos = trans.GetPosition();
            if (patch.Has(pos)) {
                const auto &delta = patch.At(pos);
                pos.line += delta.line;
                pos.column += delta.column;
            }
            return pos;
        }
        return TextPosition{};
    }

    void TransactionBatch::Finish() {
        this->stream.Commit(SlokedCursorTransaction {
            this->batch
        });
        this->batch.clear();
        this->rollback.clear();
    }
}