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

#include "sloked/text/cursor/TransactionCursor.h"

#include <functional>
#include <iostream>

#include "sloked/text/cursor/EditingPrimitives.h"
#include "sloked/text/cursor/TransactionFactory.h"

namespace sloked {

    class TransactionListener : public SlokedTransactionStream::Listener {
     public:
        TransactionListener(const Encoding &encoding,
                            TransactionCursor::Line &line,
                            TransactionCursor::Column &column)
            : encoding(encoding), line(line), column(column) {}

        void OnCommit(const SlokedCursorTransaction &trans) override {
            auto patch = trans.CommitPatch(this->encoding);
            TextPosition pos{this->line, this->column};
            if (patch.Has(pos)) {
                const auto &delta = patch.At(pos);
                this->line += delta.line;
                this->column += delta.column;
            }
        }

        void OnRollback(const SlokedCursorTransaction &trans) override {
            auto patch = trans.RollbackPatch(this->encoding);
            TextPosition pos{this->line, this->column};
            if (patch.Has(pos)) {
                const auto &delta = patch.At(pos);
                this->line += delta.line;
                this->column += delta.column;
            }
        }

        void OnRevert(const SlokedCursorTransaction &trans) override {
            auto patch = trans.CommitPatch(this->encoding);
            TextPosition pos{this->line, this->column};
            if (patch.Has(pos)) {
                const auto &delta = patch.At(pos);
                this->line += delta.line;
                this->column += delta.column;
            }
        }

     private:
        const Encoding &encoding;
        TransactionCursor::Line &line;
        TransactionCursor::Column &column;
    };

    TransactionCursor::TransactionCursor(TextBlock &text,
                                         const Encoding &encoding,
                                         SlokedTransactionStream &stream)
        : text(text), encoding(encoding), stream(stream), line(0), column(0) {
        this->listener = std::make_shared<TransactionListener>(
            this->encoding, this->line, this->column);
        this->stream.AddListener(this->listener);
    }

    TransactionCursor::~TransactionCursor() {
        this->stream.RemoveListener(*this->listener);
    }

    void TransactionCursor::Undo() {
        if (this->stream.HasRollback()) {
            auto pos = this->stream.Rollback();
            this->line = pos.line;
            this->column = pos.column;
        }
    }

    bool TransactionCursor::HasUndoable() const {
        return this->stream.HasRollback();
    }

    void TransactionCursor::Redo() {
        if (this->stream.HasRevertable()) {
            auto pos = this->stream.RevertRollback();
            this->line = pos.line;
            this->column = pos.column;
        }
    }

    bool TransactionCursor::HasRedoable() const {
        return this->stream.HasRevertable();
    }

    TextPosition::Line TransactionCursor::GetLine() const {
        return this->line;
    }

    TextPosition::Column TransactionCursor::GetColumn() const {
        return this->column;
    }

    void TransactionCursor::SetPosition(Line l, Column c) {
        if (l <= this->text.GetLastLine()) {
            auto columns = this->encoding.CodepointCount(this->text.GetLine(l));
            if (c <= columns) {
                this->line = l;
                this->column = c;
            }
        }
    }

    void TransactionCursor::MoveUp(Line l) {
        this->line -= std::min(l, this->line);
        this->column = std::min(
            this->column,
            static_cast<TextPosition::Column>(
                this->encoding.CodepointCount(this->text.GetLine(this->line))));
    }

    void TransactionCursor::MoveDown(Line l) {
        this->line += std::min(this->line + l, static_cast<TextPosition::Line>(
                                                   this->text.GetLastLine())) -
                      this->line;
        this->column = std::min(
            this->column,
            static_cast<TextPosition::Column>(
                this->encoding.CodepointCount(this->text.GetLine(this->line))));
    }

    void TransactionCursor::MoveForward(Column c) {
        this->column = std::min(
            this->column + c,
            static_cast<TextPosition::Column>(
                this->encoding.CodepointCount(this->text.GetLine(this->line))));
    }

    void TransactionCursor::MoveBackward(Column c) {
        this->column -= std::min(c, this->column);
    }

    void TransactionCursor::Insert(std::string_view view) {
        this->applyCommand(SlokedTransactionFactory::Insert(
            TextPosition{this->line, this->column}, view));
    }

    void TransactionCursor::NewLine(std::string_view view) {
        this->applyCommand(SlokedTransactionFactory::Newline(
            TextPosition{this->line, this->column}, view));
    }

    void TransactionCursor::DeleteBackward() {
        this->applyCommand(SlokedTransactionFactory::DeleteBackward(
            this->text, this->encoding,
            TextPosition{this->line, this->column}));
    }

    void TransactionCursor::DeleteForward() {
        this->applyCommand(SlokedTransactionFactory::DeleteForward(
            this->text, this->encoding,
            TextPosition{this->line, this->column}));
    }

    void TransactionCursor::ClearRegion(const TextPosition &from,
                                        const TextPosition &to) {
        this->applyCommand(SlokedTransactionFactory::ClearRegion(
            this->text, this->encoding, from, to));
    }

    void TransactionCursor::applyCommand(const SlokedCursorTransaction &trans) {
        this->stream.Commit(trans);
    }
}  // namespace sloked