#ifndef SLOKED_SCREEN_WIDGETS_TEXTEDITOR_H_
#define SLOKED_SCREEN_WIDGETS_TEXTEDITOR_H_

#include "sloked/core/CharWidth.h"
#include "sloked/core/Encoding.h"
#include "sloked/text/TextBlock.h"
#include "sloked/text/TextFrame.h"
#include "sloked/text/cursor/Cursor.h"
#include "sloked/text/cursor/TransactionJournal.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/screen/widgets/TextPane.h"
#include "sloked/screen/widgets/TextPaneWidget.h"

namespace sloked {

    class SlokedTextEditor : public SlokedTextPaneWidget {
     public:
        SlokedTextEditor(TextBlock &, SlokedCursor &, SlokedTransactionJournal &, SlokedTaggedText<int> &, const EncodingConverter &, const SlokedCharWidth &, SlokedBackgroundGraphics = SlokedBackgroundGraphics::Black);

        bool ProcessInput(const SlokedKeyboardInput &) override;
        void Render(SlokedTextPane &) override;

     private:
        TextBlock &text;
        SlokedCursor &cursor;
        SlokedTransactionJournal &journal;
        SlokedTaggedText<int> &tags;
        const EncodingConverter &conv;
        const SlokedCharWidth &charWidth;

        TextFrameView frame;
        SlokedTaggedTextView<int> tagsView;
        SlokedBackgroundGraphics background;
    };
}

#endif