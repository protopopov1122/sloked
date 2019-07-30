#ifndef SLOKED_CORE_POSITION_H_
#define SLOKED_CORE_POSITION_H_

#include "sloked/Base.h"

namespace sloked {

    struct TextPosition {
        using Line = unsigned int;
        using Column = unsigned int;

        Line line;
        Column column;

        bool operator<(const TextPosition &) const;
    };
}

#endif