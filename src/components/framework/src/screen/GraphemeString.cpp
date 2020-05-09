#include "sloked/screen/GraphemeString.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedGraphemeStringLayout::SlokedGraphemeStringLayout(std::string_view in, const Encoding &encoding,
                        const SlokedCharPreset &charPreset,
                        const SlokedGraphemeEnumerator &graphemes,
                        const SlokedFontProperties &fontProperies) {
        GraphemeBounds::Split(in, encoding, charPreset, graphemes, fontProperies, std::back_inserter(this->layout));
    }

    std::size_t SlokedGraphemeStringLayout::Count() const {
        return this->layout.size();
    }

    const GraphemeBounds &SlokedGraphemeStringLayout::At(TextPosition::Column column) const {
        if (column < this->layout.size()) {
            return this->layout.at(column);
        } else {
            throw SlokedError("GraphemeStringLayout: Out of bounds");
        }
    }

    SlokedGraphemeStringLayout::ConstIterator SlokedGraphemeStringLayout::FindByDirectOffset(TextPosition::Column offset) const {
        return GraphemeBounds::FindByDirectOffset(this->begin(), this->end(), offset);
    }

    SlokedGraphicsPoint::Coordinate SlokedGraphemeStringLayout::TotalGraphicalWidth(const ConstIterator &begin, const ConstIterator &end) const {
        return GraphemeBounds::TotalGraphicalWidth(begin, end);
    }

    SlokedGraphemeStringLayout::ConstIterator SlokedGraphemeStringLayout::begin() const {
        return this->layout.begin();
    }

    SlokedGraphemeStringLayout::ConstIterator SlokedGraphemeStringLayout::end() const {
        return this->layout.end();
    }
}