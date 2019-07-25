#ifndef SLOKED_SCREEN_TERM_MULTIPLEXER_TERMINALSCREENVIEW_H_
#define SLOKED_SCREEN_TERM_MULTIPLEXER_TERMINALSCREENVIEW_H_

#include "sloked/screen/Terminal.h"
#include <memory>

namespace sloked {

    class TerminalScreenView {
     public:
        TerminalScreenView(SlokedTerminal &);
        TerminalScreenView(SlokedTerminal &, SlokedTerminal::Column, SlokedTerminal::Line);
        TerminalScreenView(const TerminalScreenView &) = delete;
        TerminalScreenView(TerminalScreenView &&);

        TerminalScreenView &operator=(const TerminalScreenView &) = delete;
        TerminalScreenView &operator=(TerminalScreenView &&);

        bool Empty() const;
        std::unique_ptr<SlokedTerminal> NewTerminal();

     private:
        void reset();

        SlokedTerminal *term;
        SlokedTerminal::Column columns;
        SlokedTerminal::Line lines;
    };
}

#endif