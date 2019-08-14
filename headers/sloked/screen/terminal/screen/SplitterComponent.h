#ifndef SLOKED_SCREEN_TERMINAL_SCREEN_SPLITTERCOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_SCREEN_SPLITTERCOMPONENT_H_

#include "sloked/core/CharWidth.h"
#include "sloked/core/Encoding.h"
#include "sloked/screen/components/SplitterComponent.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/multiplexer/TerminalSplitter.h"
#include "sloked/screen/terminal/screen/ComponentHandle.h"
#include <vector>
#include <memory>

namespace sloked {

    class TerminalSplitterComponent : public SlokedSplitterComponent {
     public:        
        TerminalSplitterComponent(SlokedTerminal &, Splitter::Direction, const Encoding &, const SlokedCharWidth &);

        std::optional<WinId> GetFocus() const override;
        SlokedComponentHandle &GetWindow(WinId) const override;
        WinId GetWindowCount() const override;

        bool SetFocus(WinId) override;
        SlokedIndexed<SlokedComponentHandle &, WinId> NewWindow(const Splitter::Constraints &) override;
        SlokedIndexed<SlokedComponentHandle &, WinId> NewWindow(WinId, const Splitter::Constraints &) override;
        bool CloseWindow(WinId) override;

        void ProcessInput(const SlokedKeyboardInput &) override;
        void Render() override;

     private:
        TerminalSplitter splitter;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        std::vector<std::shared_ptr<TerminalComponentHandle>> components;
    };
}

#endif