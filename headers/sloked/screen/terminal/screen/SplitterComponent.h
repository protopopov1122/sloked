#ifndef SLOKED_SCREEN_TERMINAL_SCREEN_SPLITTERCOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_SCREEN_SPLITTERCOMPONENT_H_

#include "sloked/core/Encoding.h"
#include "sloked/screen/widgets/SplitterComponent.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/CharWidth.h"
#include "sloked/screen/terminal/multiplexer/TerminalSplitter.h"
#include "sloked/screen/terminal/screen/ComponentHandle.h"
#include <vector>
#include <memory>

namespace sloked {

    class TerminalSplitterComponent : public SlokedSplitterComponent {
     public:        
        TerminalSplitterComponent(SlokedTerminal &, Splitter::Direction, const Encoding &, const ScreenCharWidth &);

        void SetFocus(std::size_t) override;
        std::size_t GetFocus() const override;
        SlokedComponentHandle *GetWindow(std::size_t) const override;
        std::size_t GetWindowCount() const override;
        SlokedComponentHandle &NewWindow(const Splitter::Constraints &) override;

        void ProcessInput(const SlokedKeyboardInput &) override;
        void Render() override;

     private:
        TerminalSplitter splitter;
        const Encoding &encoding;
        const ScreenCharWidth &charWidth;
        std::vector<std::shared_ptr<TerminalComponentHandle>> components;
    };
}

#endif