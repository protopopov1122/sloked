#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_SPLITTERCOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_SPLITTERCOMPONENT_H_

#include "sloked/core/CharWidth.h"
#include "sloked/core/Encoding.h"
#include "sloked/screen/components/SplitterComponent.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/multiplexer/TerminalSplitter.h"
#include "sloked/screen/terminal/components/ComponentHandle.h"
#include <vector>
#include <memory>

namespace sloked {

    class TerminalSplitterComponent : public SlokedSplitterComponent {
     public:        
        TerminalSplitterComponent(SlokedTerminal &, Splitter::Direction, const Encoding &, const SlokedCharWidth &);

        std::optional<WinId> GetFocus() const override;
        SlokedComponentHandle &GetWindow(WinId) const override;
        WinId GetWindowCount() const override;
        const Splitter::Constraints &GetConstraints(WinId) const override;

        bool SetFocus(WinId) override;
        SlokedIndexed<SlokedComponentHandle &, WinId> NewWindow(const Splitter::Constraints &) override;
        SlokedIndexed<SlokedComponentHandle &, WinId> NewWindow(WinId, const Splitter::Constraints &) override;
        bool UpdateConstraints(WinId, const Splitter::Constraints &) override;
        bool CloseWindow(WinId) override;

        void Render() override;
        void Update() override;

     protected:
        void ProcessComponentInput(const SlokedKeyboardInput &) override;
        
     private:
        TerminalSplitter splitter;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        std::vector<std::shared_ptr<TerminalComponentHandle>> components;
        WinId focus;
    };
}

#endif