#ifndef SLOKED_SCREEN_TERM_MULTIPLEXER_TERMINALSPLITTER_H_
#define SLOKED_SCREEN_TERM_MULTIPLEXER_TERMINALSPLITTER_H_

#include "sloked/core/Encoding.h"
#include "sloked/screen/Terminal.h"
#include "sloked/screen/CharWidth.h"
#include "sloked/screen/term-multiplexer/TerminalWindow.h"
#include <vector>
#include <memory>

namespace sloked {

    class TerminalSplitter {
     public:
        enum class Direction;
        class Constraints;

        TerminalSplitter(SlokedTerminal &, Direction, const Encoding &, const ScreenCharWidth &);

        unsigned int GetMinimum() const;
        unsigned int GetMaximum() const;
        SlokedTerminal *GetWindow(std::size_t) const;
        std::size_t GetWindowCount() const;

        SlokedTerminal &NewTerminal(const Constraints &);
        void Update();

     private:
        using Window = std::pair<std::shared_ptr<TerminalWindow>, Constraints>;

        SlokedTerminal &term;
        Direction direction;
        const Encoding &encoding;
        const ScreenCharWidth &charWidth;
        std::vector<Window> windows;
    };

    enum class TerminalSplitter::Direction {
        Vertical,
        Horizontal
    };

    class TerminalSplitter::Constraints {
     public:
        Constraints(float, unsigned int = 0, unsigned int = 0);

        float GetDimensions() const;
        unsigned int GetMinimum() const;
        unsigned int GetMaximum() const;

        unsigned int Calc(unsigned int) const;
     private:
        float dim;
        unsigned int min;
        unsigned int max;
    };
}

#endif