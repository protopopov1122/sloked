#ifndef SLOKED_SCREEN_TERMINAL_SCREEN_TEXTPANECOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_SCREEN_TEXTPANECOMPONENT_H_

#include "sloked/screen/widgets/TextPaneComponent.h"
#include "sloked/screen/terminal/Terminal.h"

namespace sloked {

    class TerminalTextPaneComponent : public SlokedTextPaneComponent {
     public:
        TerminalTextPaneComponent(SlokedTerminal &);
        void ProcessInput(const SlokedKeyboardInput &) override;
        void Render() override;
        void SetRenderer(Renderer) override;

     private:
        SlokedTerminal &term;
        Renderer renderer;
    };
}

#endif