#include "sloked/screen/terminal/screen/TabberComponent.h"

namespace sloked {

    TerminalTabberComponent::TerminalTabberComponent(SlokedTerminal &term, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : tabber(term), encoding(encoding), charWidth(charWidth) {}

    SlokedComponentHandle &TerminalTabberComponent::NewTab() {
        SlokedTerminal &term = this->tabber.NewTab();
        auto component = std::make_shared<TerminalComponentHandle>(term, this->encoding, this->charWidth);
        this->components.push_back(component);
        return *component;
    }

    SlokedComponentHandle &TerminalTabberComponent::NewTab(std::size_t idx) {
        SlokedTerminal &term = this->tabber.NewTab(idx);
        auto component = std::make_shared<TerminalComponentHandle>(term, this->encoding, this->charWidth);
        this->components.insert(this->components.begin() + idx, component);
        return *component;
    }

    void TerminalTabberComponent::SelectTab(std::size_t idx) {
        this->tabber.SelectTab(idx);
    }

    std::size_t TerminalTabberComponent::GetCurrentTab() const {
        return this->tabber.GetCurrentTab();
    }

    SlokedComponentHandle *TerminalTabberComponent::GetTab(std::size_t idx) const {
        if (idx < this->components.size()) {
            return this->components.at(idx).get();
        } else {
            return nullptr;
        }
    }

    void TerminalTabberComponent::ProcessInput(const SlokedKeyboardInput &input) {
        bool processed = false;
        if (this->inputHandler) {
            processed = this->inputHandler(input);
        }
        if (!processed && this->GetCurrentTab() < this->components.size()) {
            this->components.at(this->GetCurrentTab())->ProcessInput(input);
        }
    }

    void TerminalTabberComponent::Render() {
        if (this->GetCurrentTab() < this->components.size()) {
            this->components.at(this->GetCurrentTab())->Render();
        }
    }
}