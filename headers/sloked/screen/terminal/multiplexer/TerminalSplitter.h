#ifndef SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALSPLITTER_H_
#define SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALSPLITTER_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/Indexed.h"
#include "sloked/screen/Splitter.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/core/CharWidth.h"
#include "sloked/screen/terminal/multiplexer/TerminalWindow.h"
#include <vector>
#include <memory>
#include <functional>
#include <optional>

namespace sloked {

    class TerminalSplitter {
     public:
        using WinId = std::size_t;

        TerminalSplitter(SlokedTerminal &, Splitter::Direction, const Encoding &, const SlokedCharWidth &);

        unsigned int GetMinimum() const;
        unsigned int GetMaximum() const;
        SlokedTerminal &GetTerminal(WinId) const;
        WinId GetTerminalCount() const;
        const Splitter::Constraints &GetConstraints(WinId) const;

        SlokedIndexed<SlokedTerminal &, WinId> NewTerminal(const Splitter::Constraints &);
        SlokedIndexed<SlokedTerminal &, WinId> NewTerminal(WinId, const Splitter::Constraints &);
        bool UpdateConstraints(WinId, const Splitter::Constraints &);
        bool Move(WinId, WinId);
        bool CloseTerminal(WinId);
        void Update();

     private:
        using Window = std::pair<std::shared_ptr<TerminalWindow>, Splitter::Constraints>;

        SlokedTerminal &term;
        Splitter::Direction direction;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        std::vector<Window> windows;
    };
}

#endif