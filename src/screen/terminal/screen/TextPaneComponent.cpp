#include "sloked/screen/terminal/screen/TextPaneComponent.h"
#include "sloked/screen/terminal/screen/TextPane.h"

namespace sloked {

    TerminalTextPaneComponent::TerminalTextPaneComponent(SlokedTerminal &term, std::unique_ptr<SlokedTextPaneWidget> widget)
        : term(term), widget(std::move(widget)) {}
    
    void TerminalTextPaneComponent::ProcessInput(const SlokedKeyboardInput &input) {
        bool res = false;
        if (this->inputHandler) {
            res = this->inputHandler(input);
        }
        if (!res) {
            this->widget->ProcessInput(input);
        }
    }

    void TerminalTextPaneComponent::Render() {
        TerminalTextPane screen(this->term);
        this->widget->Render(screen);
    }
}