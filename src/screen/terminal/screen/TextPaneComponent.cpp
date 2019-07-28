#include "sloked/screen/terminal/screen/TextPaneComponent.h"
#include "sloked/screen/terminal/screen/TextPane.h"

namespace sloked {

    TerminalTextPaneComponent::TerminalTextPaneComponent(SlokedTerminal &term)
        : term(term), renderer([](auto &) {}) {}
    
    void TerminalTextPaneComponent::ProcessInput(const SlokedKeyboardInput &input) {
        if (this->inputHandler) {
            this->inputHandler(input);
        }
    }

    void TerminalTextPaneComponent::Render() {
        TerminalTextPane screen(this->term);
        this->renderer(screen);
    }

    void TerminalTextPaneComponent::SetRenderer(Renderer renderer) {
        if (this->renderer) {
            this->renderer = renderer;
        }
    }
}