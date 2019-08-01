#ifndef SLOKED_TEXT_CURSOR_TRANSACTIONFACTORY_H_
#define SLOKED_TEXT_CURSOR_TRANSACTIONFACTORY_H_

#include "sloked/text/cursor/Transaction.h"

namespace sloked {

    class SlokedTransactionFactory {
     public:
        static SlokedCursorTransaction Insert(const TextPosition &, std::string_view);
        static SlokedCursorTransaction Newline(const TextPosition &, std::string_view);
        static SlokedCursorTransaction DeleteBackward(const TextBlock &, const Encoding &, const TextPosition &);
        static SlokedCursorTransaction DeleteForward(const TextBlock &, const Encoding &, const TextPosition &);
        static SlokedCursorTransaction ClearRegion(const TextBlock &, const Encoding &, const TextPosition &, const TextPosition &);
    };
}

#endif