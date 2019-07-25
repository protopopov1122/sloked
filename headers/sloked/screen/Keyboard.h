#ifndef SLOKED_SCREEN_KEYBOARD_H_
#define SLOKED_SCREEN_KEYBOARD_H_

#include "sloked/Base.h"
#include <variant>
#include <string>

namespace sloked {

    enum class SlokedControlKey {
        ArrowUp,
        ArrowDown,
        ArrowLeft,
        ArrowRight,
        Backspace,
        Delete,
        Insert,
        Escape,
        PageUp,
        PageDown,
        Home,
        End,
        Enter,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9
    };

    using SlokedKeyboardInput = std::variant<std::string, SlokedControlKey>;
}

#endif