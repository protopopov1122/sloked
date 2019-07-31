#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONCURSOR_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONCURSOR_H_

#include "sloked/core/Encoding.h"
#include "sloked/text/TextBlock.h"
#include "sloked/text/cursor/Cursor.h"
#include "sloked/text/cursor/Transaction.h"
#include "sloked/text/cursor/TransactionStream.h"
#include "sloked/text/cursor/TransactionHistory.h"
#include <vector>
#include <memory>

namespace sloked {

    class TransactionCursor : public SlokedCursor, public SlokedTransactionHistory {
     public:
        TransactionCursor(TextBlock &, const Encoding &, SlokedTransactionStream &);

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
        void applyCommand(const SlokedEditTransaction &);
    
        TextBlock &text;
        const Encoding &encoding;
        SlokedTransactionStream &stream;
        Line line;
        Column column;
    };
}

#endif