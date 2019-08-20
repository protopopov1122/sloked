#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_TEXTPANECOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_TEXTPANECOMPONENT_H_

#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/widgets/TextPaneWidget.h"
#include <memory>

namespace sloked {

    class TerminalTextPaneComponent : public SlokedScreenComponent {
     public:
        TerminalTextPaneComponent(SlokedTerminal &, std::unique_ptr<SlokedTextPaneWidget>);
        
        void Render() override;
        void UpdateDimensions() override;
        
     protected:
        void ProcessComponentInput(const SlokedKeyboardInput &) override;

     private:
        SlokedTerminal &term;
        std::unique_ptr<SlokedTextPaneWidget> widget;
    };
}

#endif