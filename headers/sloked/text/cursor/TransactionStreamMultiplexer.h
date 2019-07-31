#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONSTREAMMULTIPLEXER_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONSTREAMMULTIPLEXER_H_

#include "sloked/text/cursor/TransactionStream.h"
#include <vector>
#include <map>

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
            SlokedEditTransaction transaction;
        };

        StreamId RegisterStream();
        void RemoveStream(StreamId);
        TextPosition Commit(StreamId, const SlokedEditTransaction &);
        bool HasRollback(StreamId) const;
        TextPosition Rollback(StreamId);
        bool HasRevertable(StreamId) const;
        TextPosition RevertRollback(StreamId);

        TextBlock &text;
        const Encoding &encoding;
        StreamId nextStreamId;
        std::vector<LabeledTransaction> history;
        std::map<std::size_t, std::vector<SlokedEditTransaction>> backtrack;
    };
}

#endif