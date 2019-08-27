#ifndef SLOKED_CORE_POSITION_H_
#define SLOKED_CORE_POSITION_H_

#include "sloked/Base.h"
#include <cinttypes>

namespace sloked {

    struct TextPosition {
        using Line = uint32_t;
        using Column = uint32_t;

        Line line;
        Column column;

        bool operator<(const TextPosition &) const;
        bool operator<=(const TextPosition &) const;
        bool operator>(const TextPosition &) const;
        bool operator>=(const TextPosition &) const;
        bool operator==(const TextPosition &) const;
        bool operator!=(const TextPosition &) const;

        TextPosition operator+(const TextPosition &) const;
        TextPosition operator-(const TextPosition &) const;

        static const TextPosition Min;
        static const TextPosition Max;
    };

    struct TextPositionDelta {
        using Line = int64_t;
        using Column = int64_t;

        Line line;
        Column column;

        bool operator==(const TextPositionDelta &) const;
    };
}

#endif