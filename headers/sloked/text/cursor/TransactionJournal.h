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