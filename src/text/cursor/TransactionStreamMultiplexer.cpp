#include "sloked/text/cursor/TransactionStreamMultiplexer.h"
#include <algorithm>
#include <iostream>

namespace sloked {

    class TransactionStreamMultiplexer::Stream : public SlokedTransactionStream {
     public:
        Stream(TransactionStreamMultiplexer &multiplexer)
            : multiplexer(multiplexer) {
            this->streamId = this->multiplexer.RegisterStream();
        }

        virtual ~Stream() {
            this->multiplexer.RemoveStream(this->streamId);
        }

        TextPosition Commit(const SlokedEditTransaction &transaction) override {
            return this->multiplexer.Commit(this->streamId, transaction);
        }

        bool HasRollback() const override {
            return this->multiplexer.HasRollback(this->streamId);
        }

        TextPosition Rollback() override {
            return this->multiplexer.Rollback(this->streamId);
        }

        TextPosition RevertRollback() override {
            return this->multiplexer.RevertRollback(this->streamId);
        }

        bool HasRevertable() const override {
            return this->multiplexer.HasRevertable(this->streamId);
        }

     private:
        TransactionStreamMultiplexer &multiplexer;
        TransactionStreamMultiplexer::StreamId streamId;
    };

    TransactionStreamMultiplexer::TransactionStreamMultiplexer(TextBlock &text, const Encoding &encoding)
        : text(text), encoding(encoding), nextStreamId(0) {}

    std::unique_ptr<SlokedTransactionStream> TransactionStreamMultiplexer::NewStream() {
        return std::make_unique<Stream>(*this);
    }

    TransactionStreamMultiplexer::StreamId TransactionStreamMultiplexer::RegisterStream() {
        std::size_t id = this->nextStreamId++;
        this->backtrack[id] = std::vector<SlokedEditTransaction>{};
        return id;
    }
    
    void TransactionStreamMultiplexer::RemoveStream(StreamId id) {
        if (this->backtrack.count(id)) {
            this->backtrack.erase(id);
        }
    }

    TextPosition TransactionStreamMultiplexer::Commit(StreamId stream, const SlokedEditTransaction &transaction) {
        auto pos = transaction.Commit(this->text, this->encoding);
        if (this->backtrack.count(stream)) {
            this->backtrack[stream].clear();
        }
        auto patch = transaction.CommitPatch(this->encoding);
        this->history.push_back(LabeledTransaction {stream, transaction});
        for (auto &stream : this->backtrack) {
            for (auto &trans : stream.second) {
                trans.Apply(patch);
            }
        }
        return pos;
    }

    bool TransactionStreamMultiplexer::HasRollback(StreamId streamId) const {
        auto it = std::find_if(this->history.rbegin(), this->history.rend(), [streamId](const auto &trans) {
            return trans.stream == streamId;
        });
        return it != this->history.rend();
    }

    TextPosition TransactionStreamMultiplexer::Rollback(StreamId id) {
        std::size_t idx = this->history.size() - 1;
        for (; idx < this->history.size() && this->history[idx].stream != id; idx--) {}
        if (idx < this->history.size()) {
            TextPosition pos;
            for (std::size_t i = this->history.size() - 1; i >= idx && i < this->history.size(); i--) {
                pos = this->history[i].transaction.Rollback(this->text, this->encoding);
            }
            auto rollbacked = this->history[idx].transaction;
            this->history.erase(this->history.begin() + idx);
            auto patch = rollbacked.RollbackPatch(this->encoding);
            for (auto &stream : this->backtrack) {
                if (stream.first != id) {
                    for (auto &trans : stream.second) {
                        trans.Apply(patch);
                    }
                }
            }
            for (std::size_t i = idx; i < this->history.size(); i++) {
                this->history[i].transaction.Apply(patch);
                this->history[i].transaction.Commit(this->text, this->encoding);
                auto patch = this->history[i].transaction.CommitPatch(this->encoding);
                if (patch.Has(pos)) {
                    const auto &delta = patch.At(pos);
                    pos.line += delta.line;
                    pos.column += delta.column;
                }
                rollbacked.Apply(patch);
            }
            if (this->backtrack.count(id)) {
                this->backtrack[id].push_back(rollbacked);
            }
            return pos;
        }
        return TextPosition{};
    }
    
    bool TransactionStreamMultiplexer::HasRevertable(StreamId id) const {
        if (this->backtrack.count(id)) {
            return !this->backtrack.at(id).empty();
        } else {
            return false;
        }
    }

    TextPosition TransactionStreamMultiplexer::RevertRollback(StreamId id) {
        if (!this->backtrack[id].empty()) {
            auto transaction = this->backtrack[id].back();
            auto pos = transaction.Commit(this->text, this->encoding);
            this->backtrack[id].pop_back();
            this->history.push_back(LabeledTransaction {id, transaction});
            return pos;
        }
        return TextPosition{};
    }
}