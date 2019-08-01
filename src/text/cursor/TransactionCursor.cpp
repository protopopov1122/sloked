#include "sloked/text/cursor/TransactionCursor.h"
#include "sloked/text/cursor/EditingPrimitives.h"
#include <functional>
#include <iostream>

namespace sloked {

    class TransactionListener : public SlokedTransactionStream::Listener {
     public:
        TransactionListener(const Encoding &encoding, TransactionCursor::Line &line, TransactionCursor::Column &column)
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

    TransactionCursor::TransactionCursor(TextBlock &text, const Encoding &encoding, SlokedTransactionStream &stream)
        : text(text), encoding(encoding), stream(stream), line(0), column(0) {
        this->listener = std::make_shared<TransactionListener>(this->encoding, this->line, this->column);
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
        this->column = std::min(this->column, static_cast<TextPosition::Column>(this->encoding.CodepointCount(this->text.GetLine(this->line))));
    }

    void TransactionCursor::MoveDown(Line l) {
        this->line += std::min(this->line + l, static_cast<TextPosition::Line>(this->text.GetLastLine())) - this->line;
        this->column = std::min(this->column, static_cast<TextPosition::Column>(this->encoding.CodepointCount(this->text.GetLine(this->line))));
    }

    void TransactionCursor::MoveForward(Column c) {
        this->column = std::min(this->column + c, static_cast<TextPosition::Column>(this->encoding.CodepointCount(this->text.GetLine(this->line))));
    }

    void TransactionCursor::MoveBackward(Column c) {
        this->column -= std::min(c, this->column);
    }

    void TransactionCursor::Insert(std::string_view view) {
        this->applyCommand(SlokedCursorTransaction {
            SlokedCursorTransaction::Action::Insert,
            SlokedCursorTransaction::Content {
                TextPosition {this->line, this->column},
                std::string {view}
            }
        });
    }
    
    void TransactionCursor::NewLine(std::string_view view) {
        this->applyCommand(SlokedCursorTransaction {
            SlokedCursorTransaction::Action::Newline,
            SlokedCursorTransaction::Content {
                TextPosition {this->line, this->column},
                std::string {view}
            }
        });
    }

    void TransactionCursor::DeleteBackward() {
        std::string content = "";
        Column width = 0;
        if (this->column > 0) {
            std::string_view view = this->text.GetLine(this->line);
            auto pos = this->encoding.GetCodepoint(view, this->column - 1);
            content = view.substr(pos.first, pos.second);
        } else {
            width = this->encoding.CodepointCount(this->text.GetLine(this->line - 1));
        }
        this->applyCommand(SlokedCursorTransaction {
            SlokedCursorTransaction::Action::DeleteBackward,
            SlokedCursorTransaction::DeletePosition {
                TextPosition {this->line, this->column},
                content,
                width
            }
        });
    }

    void TransactionCursor::DeleteForward() {
        std::string content = "";
        Column width = this->encoding.CodepointCount(this->text.GetLine(this->line));
        if (this->column < width) {
            std::string_view view = this->text.GetLine(this->line);
            auto pos = this->encoding.GetCodepoint(view, this->column);
            content = view.substr(pos.first, pos.second);
        }
        this->applyCommand(SlokedCursorTransaction {
            SlokedCursorTransaction::Action::DeleteForward,
            SlokedCursorTransaction::DeletePosition {
                TextPosition {this->line, this->column},
                content,
                width
            }
        });
    }

    static TextPosition ClampPosition(const TextBlock &text, const Encoding &encoding, const TextPosition &position) {
        TextPosition res;
        res.line = std::min(position.line, static_cast<TextPosition::Line>(text.GetLastLine()));
        res.column = std::min(position.column, static_cast<TextPosition::Column>(encoding.CodepointCount(text.GetLine(res.line))));
        return res;
    }

    void TransactionCursor::ClearRegion(const TextPosition &from, const TextPosition &to) {
        auto cfrom = ClampPosition(this->text, this->encoding, from);
        auto cto = ClampPosition(this->text, this->encoding, to);
        this->applyCommand(SlokedCursorTransaction {
            SlokedCursorTransaction::Range {
                cfrom,
                cto,
                SlokedEditingPrimitives::Read(this->text, this->encoding, cfrom, cto)
            }
        });
    }

    void TransactionCursor::applyCommand(const SlokedCursorTransaction &trans) {
        auto pos = this->stream.Commit(trans);
        this->line = pos.line;
        this->column = pos.column;
    }
}