#include "sloked/core/Error.h"
#include "sloked/screen/terminal/screen/SplitterComponent.h"

namespace sloked {

    TerminalSplitterComponent::TerminalSplitterComponent(SlokedTerminal &term, Splitter::Direction dir, const Encoding &enc, const SlokedCharWidth &chWidth)
        : splitter(term, dir, enc, chWidth), encoding(enc), charWidth(chWidth) {}
    
    bool TerminalSplitterComponent::SetFocus(std::size_t idx) {
        return this->splitter.SetFocus(idx);
    }

    std::optional<TerminalSplitterComponent::WinId> TerminalSplitterComponent::GetFocus() const {
        return this->splitter.GetFocus();
    }

    SlokedComponentHandle &TerminalSplitterComponent::GetWindow(std::size_t idx) const {
        if (idx < this->components.size()) {
            return *this->components.at(idx);
        } else {
            throw SlokedError("Window #" + std::to_string(idx) + " not found");
        }
    }

    TerminalSplitterComponent::WinId TerminalSplitterComponent::GetWindowCount() const {
        return this->components.size();
    }

    SlokedIndexed<SlokedComponentHandle &, TerminalSplitterComponent::WinId> TerminalSplitterComponent::NewWindow(const Splitter::Constraints &constraints) {
        auto term = this->splitter.NewTerminal(constraints);
        auto component = std::make_shared<TerminalComponentHandle>(term.value, this->encoding, this->charWidth);
        this->components.push_back(component);
        return {term.index, *component};
    }

    SlokedIndexed<SlokedComponentHandle &, TerminalSplitterComponent::WinId> TerminalSplitterComponent::NewWindow(WinId idx, const Splitter::Constraints &constraints) {
        auto term = this->splitter.NewTerminal(idx, constraints);
        auto component = std::make_shared<TerminalComponentHandle>(term.value, this->encoding, this->charWidth);
        this->components.insert(this->components.begin() + idx, component);
        return {idx, *component};
    }
    
    bool TerminalSplitterComponent::CloseWindow(WinId idx) {
        if (idx < this->components.size()) {
            this->components.erase(this->components.begin() + idx);
            return this->splitter.CloseTerminal(idx);
        } else {
            return false;
        }
    }

    void TerminalSplitterComponent::ProcessInput(const SlokedKeyboardInput &input) {
        bool processed = false;
        if (this->inputHandler) {
            processed = this->inputHandler(input);
        }
        if (!processed && this->GetFocus().has_value()) {
            this->components.at(this->GetFocus().value())->ProcessInput(input);
        }
    }

    void TerminalSplitterComponent::Render() {
        for (std::size_t i = 0; i < this->components.size(); i++) {
            if (i != this->GetFocus()) {
                this->components.at(i)->Render();
            }
        }
        if (this->GetFocus().has_value()) {
            this->components.at(this->GetFocus().value())->Render();
        }
    }
}