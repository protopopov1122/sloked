#ifndef SLOKED_SERVICES_TEXTEDITOR_H_
#define SLOKED_SERVICES_TEXTEDITOR_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/CharWidth.h"
#include "sloked/kgr/Service.h"
#include "sloked/kgr/ContextManager.h"
#include "sloked/kgr/local/Context.h"
#include "sloked/text/TextBlock.h"
#include "sloked/text/cursor/TransactionStreamMultiplexer.h"
#include "sloked/text/fragment/TaggedText.h"

namespace sloked {

    class SlokedTextEditorService : public KgrService {
     public:
        enum class Command {
            Insert,
            MoveUp,
            MoveDown,
            MoveBackward,
            MoveForward,
            NewLine,
            DeleteBackward,
            DeleteForward,
            Undo,
            Redo,
            Render
        };

        SlokedTextEditorService(TextBlock &, const Encoding &, const SlokedCharWidth &, SlokedTextTaggerFactory<int> &,
            KgrContextManager<KgrLocalContext> &);
        bool Attach(std::unique_ptr<KgrPipe>) override;
    
     private:
        TextBlock &text;
        EncodingConverter conv;
        SlokedTextTaggerFactory<int> &taggerFactory;
        const SlokedCharWidth &charWidth;
        KgrContextManager<KgrLocalContext> &contextManager;
        TransactionStreamMultiplexer multiplexer;
    };
}

#endif