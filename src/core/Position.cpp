#include "sloked/core/Position.h"

namespace sloked {

    bool TextPosition::operator<(const TextPosition &other) const {
        return this->line < other.line || (this->line == other.line && this->column < other.column);
    }
}