#ifndef SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALTABBER_H_
#define SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALTABBER_H_

#include "sloked/screen/terminal/Terminal.h"
#include <vector>
#include <memory>

namespace sloked {

    class TerminalTabber {
     public:
        TerminalTabber(SlokedTerminal &);

        SlokedTerminal &NewTab();
        SlokedTerminal &NewTab(std::size_t);
        void SelectTab(std::size_t);
        std::size_t GetCurrentTab() const;
        SlokedTerminal *GetTab(std::size_t) const;
        
     private:
        SlokedTerminal &term;   
        std::vector<std::shared_ptr<SlokedTerminal>> tabs;
        std::size_t current_tab;
    };
}

#endif