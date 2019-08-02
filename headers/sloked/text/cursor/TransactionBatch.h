#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONBATCH_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONBATCH_H_

#include "sloked/core/Listener.h"
#include "sloked/text/cursor/TransactionStream.h"

namespace sloked {

    class TransactionBatch : public virtual SlokedTransactionStream,
                             public SlokedListenerManager<SlokedTransactionStream::Listener, SlokedCursorTransaction, SlokedTransactionStream> {
     public:
        TransactionBatch(SlokedTransactionStream &, const Encoding &);
        TextPosition Commit(const SlokedCursorTransaction &) override;
        bool HasRollback() const override;
        TextPosition Rollback() override;
        bool HasRevertable() const override;
        TextPosition RevertRollback() override;
        void Finish();

     private:
        SlokedTransactionStream &stream;
        const Encoding &encoding;
        std::vector<SlokedCursorTransaction> batch;
        std::vector<SlokedCursorTransaction> rollback;
    };
}

#endif