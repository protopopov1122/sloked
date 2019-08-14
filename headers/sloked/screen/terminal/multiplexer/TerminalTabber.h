#ifndef SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALTABBER_H_
#define SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALTABBER_H_

#include "sloked/core/Indexed.h"
#include "sloked/screen/terminal/Terminal.h"
#include <vector>
#include <memory>
#include <optional>

namespace sloked {

    class TerminalTabber {
     public:
        using TabId = std::size_t;

        TerminalTabber(SlokedTerminal &);
        
        TabId GetTabCount() const;
        std::optional<TabId> GetCurrentTab() const;
        SlokedTerminal &GetTab(TabId) const;

        SlokedIndexed<SlokedTerminal &, TabId> NewTab();
        SlokedIndexed<SlokedTerminal &, TabId> NewTab(TabId);
        bool SelectTab(TabId);
        bool CloseTab(TabId);
        
     private:
        SlokedTerminal &term;   
        std::vector<std::shared_ptr<SlokedTerminal>> tabs;
        TabId current_tab;
    };
}

#endif