#ifndef SLOKED_SCREEN_TERMINAL_SCREEN_TABBERCOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_SCREEN_TABBERCOMPONENT_H_

#include "sloked/core/CharWidth.h"
#include "sloked/core/Encoding.h"
#include "sloked/screen/components/TabberComponent.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/multiplexer/TerminalTabber.h"
#include "sloked/screen/terminal/screen/ComponentHandle.h"
#include <vector>
#include <memory>

namespace sloked {

    class TerminalTabberComponent : public SlokedTabberComponent {
     public:
        TerminalTabberComponent(SlokedTerminal &, const Encoding &, const SlokedCharWidth &);

        TabId GetTabCount() const override;
        std::optional<TabId> GetCurrentTab() const override;
        SlokedComponentHandle &GetTab(TabId) const override;

        SlokedIndexed<SlokedComponentHandle &, TabId> NewTab() override;
        SlokedIndexed<SlokedComponentHandle &, TabId> NewTab(TabId) override;
        bool SelectTab(TabId) override;
        bool CloseTab(TabId) override;

        void ProcessInput(const SlokedKeyboardInput &) override;
        void Render() override;

     private:
        TerminalTabber tabber;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        std::vector<std::shared_ptr<TerminalComponentHandle>> components;
    };
}

#endif