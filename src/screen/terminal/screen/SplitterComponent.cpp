#include "sloked/screen/terminal/screen/SplitterComponent.h"

namespace sloked {

    TerminalSplitterComponent::TerminalSplitterComponent(SlokedTerminal &term, Splitter::Direction dir, const Encoding &enc, const ScreenCharWidth &chWidth)
        : splitter(term, dir, enc, chWidth), encoding(enc), charWidth(chWidth) {}
    
    void TerminalSplitterComponent::SetFocus(std::size_t idx) {
        this->splitter.SetFocus(idx);
    }

    std::size_t TerminalSplitterComponent::GetFocus() const {
        return this->splitter.GetFocus();
    }

    SlokedComponentHandle *TerminalSplitterComponent::GetWindow(std::size_t idx) const {
        if (idx < this->components.size()) {
            return this->components.at(idx).get();
        } else {
            return nullptr;
        }
    }

    std::size_t TerminalSplitterComponent::GetWindowCount() const {
        return this->components.size();
    }

    SlokedComponentHandle &TerminalSplitterComponent::NewWindow(const Splitter::Constraints &constraints) {
        SlokedTerminal &term = this->splitter.NewTerminal(constraints);
        auto component = std::make_shared<TerminalComponentHandle>(term, this->encoding, this->charWidth);
        this->components.push_back(component);
        return *component;
    }

    void TerminalSplitterComponent::ProcessInput(const SlokedKeyboardInput &input) {
        bool processed = false;
        if (this->inputHandler) {
            processed = this->inputHandler(input);
        }
        if (!processed && this->GetFocus() < this->components.size()) {
            this->components.at(this->GetFocus())->ProcessInput(input);
        }
    }

    void TerminalSplitterComponent::Render() {
        for (std::size_t i = 0; i < this->components.size(); i++) {
            if (i != this->GetFocus()) {
                this->components.at(i)->Render();
            }
        }
        if (this->GetFocus() < this->components.size()) {
            this->components.at(this->GetFocus())->Render();
        }
    }
}