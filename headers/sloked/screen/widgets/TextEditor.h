#ifndef SLOKED_SCREEN_WIDGETS_TEXTEDITOR_H_
#define SLOKED_SCREEN_WIDGETS_TEXTEDITOR_H_

#include "sloked/Base.h"
#include "sloked/text/TextBlock.h"
#include "sloked/text/cursor/Cursor.h"
#include "sloked/screen/widgets/TextPane.h"

namespace sloked {

    class SlokedTextEditor {
     public:
        SlokedTextEditor(TextBlock &, SlokedCursor &);

        void Draw(SlokedTextPane &);
     private:
        TextBlock &text;
        SlokedCursor &cursor;
        TextPosition::Line offset;
    };
}

#endif