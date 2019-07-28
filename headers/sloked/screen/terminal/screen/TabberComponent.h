#ifndef SLOKED_SCREEN_TERMINAL_SCREEN_TABBERCOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_SCREEN_TABBERCOMPONENT_H_

#include "sloked/core/Encoding.h"
#include "sloked/screen/widgets/TabberComponent.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/CharWidth.h"
#include "sloked/screen/terminal/multiplexer/TerminalTabber.h"
#include "sloked/screen/terminal/screen/ComponentHandle.h"
#include <vector>
#include <memory>

namespace sloked {

    class TerminalTabberComponent : public SlokedTabberComponent {
     public:
        TerminalTabberComponent(SlokedTerminal &, const Encoding &, const ScreenCharWidth &);

        SlokedComponentHandle &NewTab() override;
        SlokedComponentHandle &NewTab(std::size_t) override;
        void SelectTab(std::size_t) override;
        std::size_t GetCurrentTab() const override;
        SlokedComponentHandle *GetTab(std::size_t) const override;

        void ProcessInput(const SlokedKeyboardInput &) override;
        void Render() override;

     private:
        TerminalTabber tabber;
        const Encoding &encoding;
        const ScreenCharWidth &charWidth;
        std::vector<std::shared_ptr<TerminalComponentHandle>> components;
    };
}

#endif