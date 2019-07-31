#include "sloked/text/cursor/TransactionBatch.h"

namespace sloked {

    TransactionBatch::TransactionBatch(SlokedTransactionStream &stream, const TextPosition &position)
        : stream(stream), start(true), position(position) {}

    TextPosition TransactionBatch::Commit(const SlokedEditTransaction &trans) {
        this->rollback.clear();
        if (!this->start) {
            this->stream.Rollback();
        }
        this->batch.push_back(trans);
        this->start = false;
        return this->stream.Commit(SlokedEditTransaction {
            SlokedEditTransaction::Action::Batch,
            std::make_pair(this->position, this->batch)
        });
    }

    bool TransactionBatch::HasRollback() const {
        return !this->batch.empty();
    }

    TextPosition TransactionBatch::Rollback() {
        if (!this->batch.empty()) {
            auto trans = this->batch.back();
            this->rollback.push_back(trans);
            if (!this->start) {
                this->stream.Rollback();
            }
            this->batch.pop_back();
            this->start = false;
            return this->stream.Commit(SlokedEditTransaction {
                SlokedEditTransaction::Action::Batch,
                std::make_pair(this->position, this->batch)
            });
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
            if (!this->start) {
                this->stream.Rollback();
            }
            this->start = false;
            return this->stream.Commit(SlokedEditTransaction {
                SlokedEditTransaction::Action::Batch,
                std::make_pair(this->position, this->batch)
            });
        }
        return TextPosition{};
    }

    void TransactionBatch::Finish() {
        this->batch.clear();
        this->start = false;
    }

    void TransactionBatch::Finish(const TextPosition &position) {
        this->Finish();
        this->position = position;
    }
}