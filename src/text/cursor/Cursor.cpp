#include "sloked/text/cursor/Cursor.h"

namespace sloked {

    void SlokedCursor::ClearRegion(const TextPosition &to) {
        this->ClearRegion(TextPosition {this->GetLine(), this->GetColumn()}, to);
    }
}