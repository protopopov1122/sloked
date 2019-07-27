#ifndef SLOKED_SCREEN_TERM_MULTIPLEXER_GRAPHICS_H_
#define SLOKED_SCREEN_TERM_MULTIPLEXER_GRAPHICS_H_

#include "sloked/screen/Terminal.h"
#include <set>
#include <optional>
#include <bitset>
#include <limits>

namespace sloked {

    class BufferedGraphicsMode {
     public:
        void SetGraphicsMode(SlokedTerminalText);
        void SetGraphicsMode(SlokedTerminalBackground);
        void SetGraphicsMode(SlokedTerminalForeground);
        void apply(SlokedTerminal &) const;
        bool operator==(const BufferedGraphicsMode &) const;

     private:
        std::bitset<static_cast<int>(SlokedTerminalText::Count)> text;
        uint32_t background = None;
        uint32_t foreground = None;

        static constexpr std::size_t None = std::numeric_limits<uint32_t>::max();
    };
}

#endif