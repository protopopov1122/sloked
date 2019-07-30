#ifndef SLOKED_SCREEN_GRAPHICS_H_
#define SLOKED_SCREEN_GRAPHICS_H_

#include "sloked/Base.h"

namespace sloked {

    enum class SlokedTextGraphics {
        Off = 0,
        Bold,
        Underscore,
        Blink,
        Reverse,
        Concealed,
        Count
    };

    enum class SlokedForegroundGraphics {
        Black = 0,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White
    };

    enum class SlokedBackgroundGraphics {
        Black = 0,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White
    };
}

#endif