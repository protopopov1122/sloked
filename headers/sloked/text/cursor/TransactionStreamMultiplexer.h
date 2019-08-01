#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONSTREAMMULTIPLEXER_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONSTREAMMULTIPLEXER_H_

#include "sloked/text/cursor/TransactionStream.h"
#include <vector>
#include <map>
#include <utility>

namespace sloked {

    class TransactionStreamMultiplexer {
     public:
        TransactionStreamMultiplexer(TextBlock &, const Encoding &);
        std::unique_ptr<SlokedTransactionStream> NewStream();

        class Stream;
        friend class Stream;

     private:
        using StreamId = std::size_t;
        struct LabeledTransaction {
            StreamId stream;
            std::size_t stamp;
            SlokedCursorTransaction transaction;
        };

        StreamId RegisterStream(SlokedTransactionStream::Listener &);
        void RemoveStream(StreamId);
        TextPosition Commit(StreamId, const SlokedCursorTransaction &);
        bool HasRollback(StreamId) const;
        TextPosition Rollback(StreamId);
        bool HasRevertable(StreamId) const;
        TextPosition RevertRollback(StreamId);
        void TriggerListeners(void (SlokedTransactionStream::Listener::*)(const SlokedCursorTransaction &), const SlokedCursorTransaction &);

        TextBlock &text;
        const Encoding &encoding;
        StreamId nextStreamId;
        std::size_t nextTransactionStamp;
        std::vector<LabeledTransaction> journal;
        std::map<std::size_t, std::vector<LabeledTransaction>> backtrack;
        std::map<StreamId, std::reference_wrapper<SlokedTransactionStream::Listener>> listeners;
    };
}

#endif