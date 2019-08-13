#ifndef SLOKED_SCREEN_TERMINAL_SCREEN_TEXTPANECOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_SCREEN_TEXTPANECOMPONENT_H_

#include "sloked/screen/widgets/TextPaneComponent.h"
#include "sloked/screen/terminal/Terminal.h"
#include <memory>

namespace sloked {

    class TerminalTextPaneComponent : public SlokedTextPaneComponent {
     public:
        TerminalTextPaneComponent(SlokedTerminal &, std::unique_ptr<SlokedTextPaneWidget>);
        void ProcessInput(const SlokedKeyboardInput &) override;
        void Render() override;

     private:
        SlokedTerminal &term;
        std::unique_ptr<SlokedTextPaneWidget> widget;
    };
}

#endif