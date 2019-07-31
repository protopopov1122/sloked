#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONSTREAM_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONSTREAM_H_

#include "sloked/text/cursor/Transaction.h"

namespace sloked {

    class SlokedTransactionStream {
     public:
        virtual ~SlokedTransactionStream() = default;
        virtual TextPosition Commit(const SlokedEditTransaction &) = 0;
        virtual bool HasRollback() const = 0;
        virtual TextPosition Rollback() = 0;
        virtual bool HasRevertable() const = 0;
        virtual TextPosition RevertRollback() = 0;
    };
}

#endif