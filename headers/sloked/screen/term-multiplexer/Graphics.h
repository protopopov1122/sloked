#ifndef SLOKED_SCREEN_TERM_MULTIPLEXER_GRAPHICS_H_
#define SLOKED_SCREEN_TERM_MULTIPLEXER_GRAPHICS_H_

#include "sloked/screen/Terminal.h"
#include <set>
#include <optional>

namespace sloked {

    class BufferedGraphicsMode {
     public:
        void SetGraphicsMode(SlokedTerminalText);
        void SetGraphicsMode(SlokedTerminalBackground);
        void SetGraphicsMode(SlokedTerminalForeground);
        void apply(SlokedTerminal &);
        bool operator==(const BufferedGraphicsMode &);

     private:
        std::set<SlokedTerminalText> text;
        std::optional<SlokedTerminalBackground> background;
        std::optional<SlokedTerminalForeground> foreground;
    };
}

#endif