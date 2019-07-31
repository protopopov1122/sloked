#include "sloked/screen/widgets/TextEditor.h"
#include <vector>

namespace sloked {

    SlokedTextEditor::SlokedTextEditor(TextBlock &text, SlokedCursor &cursor)
        : text(text), cursor(cursor), offset(0) {}

    void SlokedTextEditor::Draw(SlokedTextPane &pane) {

    }

    // void SlokedTextEditor::Format(SlokedTextPane &pane, TextBlock &text, SlokedCursor &cursor) {
    //     auto pane_width = pane.GetWidth();
    //     auto pane_height = pane.GetHeight();
    //     TextPosition::Column max_line_width = 0;
    //     std::vector<std::string> content;

    //     auto from  = cur
    //     text.Visit()
    // }
}