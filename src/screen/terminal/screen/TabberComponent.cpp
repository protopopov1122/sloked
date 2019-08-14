#include "sloked/core/Error.h"
#include "sloked/screen/terminal/screen/TabberComponent.h"

namespace sloked {

    TerminalTabberComponent::TerminalTabberComponent(SlokedTerminal &term, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : tabber(term), encoding(encoding), charWidth(charWidth) {}

    TerminalTabberComponent::TabId TerminalTabberComponent::GetTabCount() const {
        return this->components.size();
    }

    std::optional<TerminalTabberComponent::TabId> TerminalTabberComponent::GetCurrentTab() const {
        return this->tabber.GetCurrentTab();
    }

    SlokedComponentHandle &TerminalTabberComponent::GetTab(std::size_t idx) const {
        if (idx < this->components.size()) {
            return *this->components.at(idx);
        } else {
            throw SlokedError("Tab #" + std::to_string(idx) + " not found");
        }
    }

    SlokedIndexed<SlokedComponentHandle &, TerminalTabberComponent::TabId> TerminalTabberComponent::NewTab() {
        auto term = this->tabber.NewTab();
        auto component = std::make_shared<TerminalComponentHandle>(term.value, this->encoding, this->charWidth);
        this->components.push_back(component);
        return {term.index, *component};
    }

    SlokedIndexed<SlokedComponentHandle &, TerminalTabberComponent::TabId> TerminalTabberComponent::NewTab(TabId idx) {
        auto term = this->tabber.NewTab(idx);
        auto component = std::make_shared<TerminalComponentHandle>(term.value, this->encoding, this->charWidth);
        this->components.insert(this->components.begin() + idx, component);
        return {idx, *component};
    }

    bool TerminalTabberComponent::SelectTab(TabId idx) {
        return this->tabber.SelectTab(idx);
    }

    bool TerminalTabberComponent::CloseTab(TabId idx) {
        if (idx < this->components.size()) {
            this->components.erase(this->components.begin() + idx);
            return this->tabber.CloseTab(idx);
        } else {
            return false;
        }
    }

    void TerminalTabberComponent::ProcessInput(const SlokedKeyboardInput &input) {
        bool processed = false;
        if (this->inputHandler) {
            processed = this->inputHandler(input);
        }
        if (!processed && this->GetCurrentTab().has_value()) {
            this->components.at(this->GetCurrentTab().value()) ->ProcessInput(input);
        }
    }

    void TerminalTabberComponent::Render() {
        if (this->GetCurrentTab().has_value()) {
            this->components.at(this->GetCurrentTab().value())->Render();
        }
    }
}