#ifndef SLOKED_EDITOR_DOCUMENT_H_
#define SLOKED_EDITOR_DOCUMENT_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/NewLine.h"
#include "sloked/text/TextDocument.h"
#include "sloked/text/TextChunk.h"
#include "sloked/text/cursor/TransactionStreamMultiplexer.h"
#include "sloked/namespace/Object.h"

namespace sloked {

    class SlokedEditorDocument {
     public:
        SlokedEditorDocument(SlokedNamespace &, const SlokedPath &, const Encoding &, std::unique_ptr<NewLine>);
        TextBlock &GetText();
        const Encoding &GetEncoding();
        std::unique_ptr<SlokedTransactionStream> NewStream();
        SlokedTransactionListenerManager &GetTransactionListeners();

     private:
        std::unique_ptr<SlokedIOView> fileView;
        TextChunkFactory blockFactory;
        TextDocument text;
        const Encoding &encoding;
        std::unique_ptr<NewLine> newline;
        TransactionStreamMultiplexer multiplexer;
    };
}

#endif