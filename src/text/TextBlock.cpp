#include "sloked/text/TextBlock.h"

namespace sloked {

    void TextBlockView::Visit(std::size_t start, std::size_t count, Visitor visitor) const {
        for (std::size_t i = start; i < start + count; i++) {
            visitor(this->GetLine(i));
        }
    }

    void TextBlock::Optimize() {}
}