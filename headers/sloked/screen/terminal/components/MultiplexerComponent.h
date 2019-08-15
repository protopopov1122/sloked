#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_MULTIPLEXERCOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_MULTIPLEXERCOMPONENT_H_

#include "sloked/screen/components/MultiplexerComponent.h"
#include "sloked/screen/terminal/multiplexer/TerminalWindow.h"
#include "sloked/screen/terminal/components/ComponentHandle.h"
#include <vector>
#include <memory>

namespace sloked {

    class TerminalMultiplexerComponent : public SlokedMultiplexerComponent {
     public:
        TerminalMultiplexerComponent(SlokedTerminal &, const Encoding &, const SlokedCharWidth &);

        std::optional<WinId> GetFocus() const override;
        SlokedComponentHandle &GetWindow(WinId) const override;
        WinId GetWindowCount() const override;

        bool SetFocus(WinId) override;
        SlokedIndexed<SlokedComponentHandle &, WinId> NewWindow(const TextPosition &, const TextPosition &) override;
        bool CloseWindow(WinId) override;

        void Render() override;
        void Update() override;

     protected:
        void ProcessComponentInput(const SlokedKeyboardInput &) override;
        
     private:
        SlokedTerminal &terminal;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        std::vector<std::pair<std::shared_ptr<TerminalComponentHandle>, std::shared_ptr<SlokedTerminal>>> windows;
        WinId focus;
    };
}

#endif