#include "sloked/screen/terminal/components/ComponentHandle.h"
#include "sloked/screen/terminal/components/SplitterComponent.h"
#include "sloked/screen/terminal/components/TabberComponent.h"
#include "sloked/screen/terminal/components/MultiplexerComponent.h"

namespace sloked {

    TerminalComponentHandle::TerminalComponentHandle(SlokedTerminal &term, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : terminal(term), encoding(encoding), charWidth(charWidth), component(nullptr) {}

    SlokedScreenComponent &TerminalComponentHandle::NewTextPane(std::unique_ptr<SlokedTextPaneWidget> widget) {
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

    SlokedMultiplexerComponent &TerminalComponentHandle::NewMultiplexer() {
        auto component = std::make_unique<TerminalMultiplexerComponent>(this->terminal, this->encoding, this->charWidth);
        TerminalMultiplexerComponent &ref = *component;
        this->component = std::move(component);
        return ref;
    }

    void TerminalComponentHandle::Render() {
        if (this->component) {
            this->component->Render();
        }
    }

    void TerminalComponentHandle::UpdateDimensions() {
        this->terminal.Update();
        if (this->component) {
            this->component->UpdateDimensions();
        }
    }

    void TerminalComponentHandle::ProcessComponentInput(const SlokedKeyboardInput &input) {
        if (this->component) {
            this->component->ProcessInput(input);
        }
    }
}