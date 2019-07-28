#ifndef SLOKED_SCREEN_TERMINAL_MULTIPLEXER_BUFFEREDGRAPHICS_H_
#define SLOKED_SCREEN_TERMINAL_MULTIPLEXER_BUFFEREDGRAPHICS_H_

#include "sloked/screen/Graphics.h"
#include <set>
#include <optional>
#include <bitset>
#include <limits>

namespace sloked {

    class BufferedGraphicsMode {
     public:
        void SetGraphicsMode(SlokedTextGraphics);
        void SetGraphicsMode(SlokedBackgroundGraphics);
        void SetGraphicsMode(SlokedForegroundGraphics);
        void apply(SlokedTerminal &) const;
        bool operator==(const BufferedGraphicsMode &) const;

     private:
        std::bitset<static_cast<int>(SlokedTextGraphics::Count)> text;
        uint32_t background = None;
        uint32_t foreground = None;

        static constexpr std::size_t None = std::numeric_limits<uint32_t>::max();
    };
}

#endif