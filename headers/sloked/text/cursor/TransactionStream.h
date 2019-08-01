#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONSTREAM_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONSTREAM_H_

#include "sloked/text/cursor/Transaction.h"
#include <memory>

namespace sloked {

    class SlokedTransactionStream {
     public:
        class Listener {
         public:
            virtual ~Listener() = default;
            virtual void OnCommit(const SlokedCursorTransaction &) = 0;
            virtual void OnRollback(const SlokedCursorTransaction &) = 0;
            virtual void OnRevert(const SlokedCursorTransaction &) = 0;
        };

        virtual ~SlokedTransactionStream() = default;
        virtual TextPosition Commit(const SlokedCursorTransaction &) = 0;
        virtual bool HasRollback() const = 0;
        virtual TextPosition Rollback() = 0;
        virtual bool HasRevertable() const = 0;
        virtual TextPosition RevertRollback() = 0;

        virtual void AddListener(std::shared_ptr<Listener>) = 0;
        virtual void RemoveListener(const Listener &) = 0;
        virtual void ClearListeners() = 0;
    };
}

#endif