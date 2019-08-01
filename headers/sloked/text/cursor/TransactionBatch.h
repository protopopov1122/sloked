#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONBATCH_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONBATCH_H_

#include "sloked/text/cursor/TransactionStream.h"

namespace sloked {

    class TransactionBatch : public virtual SlokedTransactionStream {
     public:
        TransactionBatch(SlokedTransactionStream &, const TextPosition &);
        TextPosition Commit(const SlokedCursorTransaction &) override;
        bool HasRollback() const override;
        TextPosition Rollback() override;
        bool HasRevertable() const override;
        TextPosition RevertRollback() override;
        void Finish();
        void Finish(const TextPosition &);

        void AddListener(std::shared_ptr<Listener>) override;
        void RemoveListener(const Listener &) override;
        void ClearListeners() override;

     private:
        SlokedTransactionStream &stream;
        bool start;
        TextPosition position;
        std::vector<SlokedCursorTransaction> batch;
        std::vector<SlokedCursorTransaction> rollback;
    };
}

#endif