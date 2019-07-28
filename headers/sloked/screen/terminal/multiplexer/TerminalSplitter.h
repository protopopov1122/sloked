#ifndef SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALSPLITTER_H_
#define SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALSPLITTER_H_

#include "sloked/core/Encoding.h"
#include "sloked/screen/Splitter.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/CharWidth.h"
#include "sloked/screen/terminal/multiplexer/TerminalWindow.h"
#include <vector>
#include <memory>
#include <functional>

namespace sloked {

    class TerminalSplitter {
     public:
        TerminalSplitter(SlokedTerminal &, Splitter::Direction, const Encoding &, const ScreenCharWidth &);

        void SetFocus(std::size_t);
        std::size_t GetFocus() const;
        unsigned int GetMinimum() const;
        unsigned int GetMaximum() const;
        SlokedTerminal *GetTerminal(std::size_t) const;
        std::size_t GetTerminalCount() const;

        SlokedTerminal &NewTerminal(const Splitter::Constraints &);
        void Update();

     private:
        using Window = std::pair<std::shared_ptr<TerminalWindow>, Splitter::Constraints>;

        SlokedTerminal &term;
        Splitter::Direction direction;
        const Encoding &encoding;
        const ScreenCharWidth &charWidth;
        std::vector<Window> windows;
        std::size_t focus;
    };
}

#endif