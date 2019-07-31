#include "sloked/core/Position.h"

namespace sloked {

    bool TextPosition::operator<(const TextPosition &other) const {
        return this->line < other.line ||
            (this->line == other.line && this->column < other.column);
    }

    bool TextPositionDelta::operator==(const TextPositionDelta &delta) const {
        return this->line == delta.line &&
            this->column == delta.column;
    }
}