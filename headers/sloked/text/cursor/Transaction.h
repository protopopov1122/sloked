#ifndef SLOKED_TEXT_CURSOR_TRANSACTION_H_
#define SLOKED_TEXT_CURSOR_TRANSACTION_H_

#include "sloked/Base.h"
#include "sloked/core/RangeMap.h"
#include "sloked/core/Position.h"
#include "sloked/core/Encoding.h"
#include "sloked/text/TextBlock.h"
#include <string>
#include <variant>

namespace sloked {

    class SlokedTransactionPatch {
     public:
        SlokedTransactionPatch();
        void NextTransaction();
        void Insert(const TextPosition &, const TextPosition &, const TextPositionDelta &);
        bool Has(const TextPosition &) const;
        TextPositionDelta At(const TextPosition &) const;

     private:
        std::vector<RangeMap<TextPosition, TextPositionDelta>> patch;
    };

    struct SlokedEditTransaction {
        enum class Action {
            Insert,
            Newline,
            DeleteForward,
            DeleteBackward,
            Clear,
            Batch
        };

        struct DeletePosition {
            TextPosition position;
            std::string content;
            TextPosition::Column width;
        };

        struct Content {
            TextPosition position;
            std::string content;
        };

        struct Range {
            TextPosition from;
            TextPosition to;
            std::vector<std::string> content;
        };

        TextPosition Commit(TextBlock &, const Encoding &) const;
        TextPosition Rollback(TextBlock &, const Encoding &) const;
        SlokedTransactionPatch CommitPatch(const Encoding &) const;
        SlokedTransactionPatch RollbackPatch(const Encoding &) const;
        void Apply(const SlokedTransactionPatch &);

        Action action;
        std::variant<DeletePosition, Content, Range, std::pair<TextPosition, std::vector<SlokedEditTransaction>>> argument;

     private:
        void CommitPatch(const Encoding &, SlokedTransactionPatch &) const;
        void RollbackPatch(const Encoding &, SlokedTransactionPatch &) const;
    };
}

#endif