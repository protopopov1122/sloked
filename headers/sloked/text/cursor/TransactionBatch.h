#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONBATCH_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONBATCH_H_

#include "sloked/text/cursor/TransactionStream.h"

namespace sloked {

    class TransactionBatch : public SlokedTransactionStream {
     public:
        TransactionBatch(SlokedTransactionStream &, const TextPosition &);
        TextPosition Commit(const SlokedEditTransaction &) override;
        bool HasRollback() const override;
        TextPosition Rollback() override;
        bool HasRevertable() const override;
        TextPosition RevertRollback() override;
        void Finish();
        void Finish(const TextPosition &);

     private:
        SlokedTransactionStream &stream;
        bool start;
        TextPosition position;
        std::vector<SlokedEditTransaction> batch;
        std::vector<SlokedEditTransaction> rollback;
    };
}

#endif