#include "sloked/screen/Splitter.h"
#include <cassert>

namespace sloked {

    Splitter::Constraints::Constraints(float dim, unsigned int min, unsigned int max)
        : dim(dim), min(min), max(max) {
        assert(dim >= 0.0f);
        assert(dim <= 1.0f);
        assert(min <= max || max == 0);
    }

    float Splitter::Constraints::GetDimensions() const {
        return this->dim;
    }

    unsigned int Splitter::Constraints::GetMinimum() const {
        return this->min;
    }

    unsigned int Splitter::Constraints::GetMaximum() const {
        return this->max;
    }
}