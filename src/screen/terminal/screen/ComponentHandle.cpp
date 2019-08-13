#include "sloked/screen/terminal/screen/ComponentHandle.h"
#include "sloked/screen/terminal/screen/SplitterComponent.h"
#include "sloked/screen/terminal/screen/TabberComponent.h"

namespace sloked {

    TerminalComponentHandle::TerminalComponentHandle(SlokedTerminal &term, const Encoding &encoding, const ScreenCharWidth &charWidth)
        : terminal(term), encoding(encoding), charWidth(charWidth), component(nullptr) {}

    void TerminalComponentHandle::ProcessInput(const SlokedKeyboardInput &input) {
        bool processed = false;
        if (this->inputHandler) {
            processed = this->inputHandler(input);
        }
        if (!processed && this->component) {
            this->component->ProcessInput(input);
        }
    }

    void TerminalComponentHandle::Render() {
        if (this->component) {
            this->component->Render();
        }
    }

    SlokedTextPaneComponent &TerminalComponentHandle::NewTextPane(std::unique_ptr<SlokedTextPaneWidget> widget) {
        auto component = std::make_unique<TerminalTextPaneComponent>(this->terminal, std::move(widget));
        TerminalTextPaneComponent &ref = *component;
        this->component = std::move(component);
        return ref;
    }

    SlokedSplitterComponent &TerminalComponentHandle::NewSplitter(Splitter::Direction dir) {
        auto component = std::make_unique<TerminalSplitterComponent>(this->terminal, dir, this->encoding, this->charWidth);
        TerminalSplitterComponent &ref = *component;
        this->component = std::move(component);
        return ref;
    }

    SlokedTabberComponent &TerminalComponentHandle::NewTabber() {
        auto component = std::make_unique<TerminalTabberComponent>(this->terminal, this->encoding, this->charWidth);
        TerminalTabberComponent &ref = *component;
        this->component = std::move(component);
        return ref;
    }
}