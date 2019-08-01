#include "sloked/text/cursor/TransactionStreamMultiplexer.h"
#include "sloked/core/Listener.h"
#include <algorithm>
#include <iostream>

namespace sloked {

    class TransactionStreamMultiplexer::Stream : public virtual SlokedTransactionStream,
                                                 public SlokedListenerManager<SlokedTransactionStream::Listener, SlokedCursorTransaction, SlokedTransactionStream>,
                                                 public SlokedTransactionStream::Listener {
     public:
        Stream(TransactionStreamMultiplexer &multiplexer)
            : multiplexer(multiplexer) {
            this->streamId = this->multiplexer.RegisterStream(*this);
        }

        virtual ~Stream() {
            this->multiplexer.RemoveStream(this->streamId);
        }

        TextPosition Commit(const SlokedCursorTransaction &transaction) override {
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

        void OnCommit(const SlokedCursorTransaction &trans) override {
            this->TriggerListeners(&SlokedTransactionStream::Listener::OnCommit, trans);
        }

        void OnRollback(const SlokedCursorTransaction &trans) override {
            this->TriggerListeners(&SlokedTransactionStream::Listener::OnRollback, trans);
        }

        void OnRevert(const SlokedCursorTransaction &trans) override {
            this->TriggerListeners(&SlokedTransactionStream::Listener::OnRevert, trans);
        }

     private:
        TransactionStreamMultiplexer &multiplexer;
        TransactionStreamMultiplexer::StreamId streamId;
    };

    TransactionStreamMultiplexer::TransactionStreamMultiplexer(TextBlock &text, const Encoding &encoding)
        : text(text), encoding(encoding), nextStreamId(0), nextTransactionStamp(0) {}

    std::unique_ptr<SlokedTransactionStream> TransactionStreamMultiplexer::NewStream() {
        return std::make_unique<Stream>(*this);
    }

    TransactionStreamMultiplexer::StreamId TransactionStreamMultiplexer::RegisterStream(SlokedTransactionStream::Listener &listener) {
        std::size_t id = this->nextStreamId++;
        this->backtrack[id] = std::vector<LabeledTransaction>{};
        this->listeners.emplace(id, std::ref(listener));
        return id;
    }
    
    void TransactionStreamMultiplexer::RemoveStream(StreamId id) {
        if (this->backtrack.count(id)) {
            this->backtrack.erase(id);
        }
        if (this->listeners.count(id)) {
            this->listeners.erase(id);
        }
    }

    TextPosition TransactionStreamMultiplexer::Commit(StreamId stream, const SlokedCursorTransaction &transaction) {
        this->journal.push_back(LabeledTransaction {
            stream,
            this->nextTransactionStamp++,
            transaction
        });
        if (this->backtrack.count(stream)) {
            this->backtrack[stream].clear();
        }
        auto pos = transaction.Commit(this->text, this->encoding);
        this->TriggerListeners(&SlokedTransactionStream::Listener::OnCommit, transaction);
        return pos;
    }

    
    bool TransactionStreamMultiplexer::HasRollback(StreamId streamId) const {
        auto it = std::find_if(this->journal.rbegin(), this->journal.rend(), [streamId](const auto &trans) {
            return trans.stream == streamId;
        });
        return it != this->journal.rend();
    }

    TextPosition TransactionStreamMultiplexer::Rollback(StreamId streamId) {
        std::size_t idx;
        for (idx = 0; idx < this->journal.size(); idx++) {
            if (this->journal[this->journal.size() - idx - 1].stream == streamId) {
                break;
            }
        }
        if (idx == this->journal.size()) {
            return TextPosition{};
        }
        idx = this->journal.size() - idx - 1;

        this->backtrack[streamId].push_back(this->journal[idx]);
        TextPosition pos;
        for (std::size_t i = this->journal.size() - 1; i >= idx && i < this->journal.size(); i--) {
            pos = this->journal[i].transaction.Rollback(this->text, this->encoding);
        }
        auto patch = this->journal[idx].transaction.RollbackPatch(this->encoding);
        this->journal.erase(this->journal.begin() + idx);
        for (std::size_t i = idx; i < this->journal.size(); i++) {
            this->journal[i].transaction.Apply(patch);
            this->journal[i].transaction.Update(this->text, this->encoding);
            this->journal[i].transaction.Commit(this->text, this->encoding);
            auto commitPatch = this->journal[i].transaction.CommitPatch(this->encoding);
            if (commitPatch.Has(pos)) {
                const auto &delta = commitPatch.At(pos);
                pos.line += delta.line;
                pos.column += delta.column;
            }
        }

        this->TriggerListeners(&SlokedTransactionStream::Listener::OnRollback, this->backtrack[streamId].back().transaction);
        return pos;
    }

    bool TransactionStreamMultiplexer::HasRevertable(StreamId id) const {
        if (this->backtrack.count(id)) {
            return !this->backtrack.at(id).empty();
        } else {
            return false;
        }
    }

    TextPosition TransactionStreamMultiplexer::RevertRollback(StreamId streamId) {
        if (this->backtrack.count(streamId) == 0 || this->backtrack[streamId].empty()) {
            return TextPosition{};
        }
        auto transaction = this->backtrack[streamId].back();
        this->backtrack[streamId].pop_back();

        std::size_t idx;
        for (idx = 0; idx < this->journal.size(); idx++) {
            if (this->journal[this->journal.size() - idx - 1].stamp < transaction.stamp) {
                break;
            }
        }
        idx = this->journal.size() - idx;

        for (std::size_t i = this->journal.size() - 1; i >= idx && i < this->journal.size(); i--) {
            this->journal[i].transaction.Rollback(this->text, this->encoding);
        }
        auto patch = transaction.transaction.CommitPatch(this->encoding);
        if (idx < this->journal.size()) {
            this->journal.insert(this->journal.begin() + idx, transaction);
        } else {
            this->journal.push_back(transaction);
        }
        TextPosition pos = transaction.transaction.Commit(this->text, this->encoding);
        for (std::size_t i = idx + 1; i < this->journal.size(); i++) {
            this->journal[i].transaction.Apply(patch);
            this->journal[i].transaction.Update(this->text, this->encoding);
            this->journal[i].transaction.Commit(this->text, this->encoding);
            auto commitPatch = this->journal[i].transaction.CommitPatch(this->encoding);
            if (commitPatch.Has(pos)) {
                const auto &delta = commitPatch.At(pos);
                pos.line += delta.line;
                pos.column += delta.column;
            }
        }

        this->TriggerListeners(&SlokedTransactionStream::Listener::OnRevert, transaction.transaction);
        return pos;
    }

    void TransactionStreamMultiplexer::TriggerListeners(void (SlokedTransactionStream::Listener::*callback)(const SlokedCursorTransaction &), const SlokedCursorTransaction &trans) {
        for (auto listener : this->listeners) {
            (listener.second.get().*callback)(trans);
        }
    }
}