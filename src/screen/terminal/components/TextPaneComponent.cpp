#include "sloked/screen/terminal/components/TextPaneComponent.h"
#include "sloked/screen/terminal/components/TextPane.h"

namespace sloked {

    TerminalTextPaneComponent::TerminalTextPaneComponent(SlokedTerminal &term, std::unique_ptr<SlokedTextPaneWidget> widget)
        : term(term), widget(std::move(widget)) {}

    void TerminalTextPaneComponent::Render() {
        if (this->widget) {
            TerminalTextPane screen(this->term);
            this->widget->Render(screen);
        }
    }

    void TerminalTextPaneComponent::UpdateDimensions() {}
    
    void TerminalTextPaneComponent::ProcessComponentInput(const SlokedKeyboardInput &input) {
        if (this->widget) {
            this->widget->ProcessInput(input);
        }
    }
}