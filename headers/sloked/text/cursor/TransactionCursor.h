#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONCURSOR_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONCURSOR_H_

#include "sloked/text/cursor/Cursor.h"
#include "sloked/text/cursor/Reader.h"
#include "sloked/text/cursor/TransactionHistory.h"
#include <vector>
#include <memory>

namespace sloked {

    class TransactionCursor : public SlokedCursor, public SlokedTransactionHistory {
     public:
        TransactionCursor(SlokedCursor &, const SlokedTextReader &);

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

        class Command;

     private:
        void applyCommand(std::shared_ptr<Command>);
    
        SlokedCursor &base;
        const SlokedTextReader &reader;
        std::vector<std::shared_ptr<Command>> history;
        std::vector<std::shared_ptr<Command>> redo;
    };
}

#endif