#include "sloked/core/Error.h"
#include "sloked/screen/terminal/screen/MultiplexerComponent.h"

namespace sloked {

    TerminalMultiplexerComponent::TerminalMultiplexerComponent(SlokedTerminal &term, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : terminal(term), encoding(encoding), charWidth(charWidth), focus(0) {}

    std::optional<TerminalMultiplexerComponent::WinId> TerminalMultiplexerComponent::GetFocus() const {
        if (this->focus < this->windows.size()) {
            return this->focus;
        } else {
            return std::optional<TerminalMultiplexerComponent::WinId>{};
        }
    }

    SlokedComponentHandle &TerminalMultiplexerComponent::GetWindow(WinId idx) const {
        if (idx < this->windows.size()) {
            return *this->windows.at(idx).first;
        } else {
            throw SlokedError("Window #" + std::to_string(idx) + " not found");
        }
    }

    TerminalMultiplexerComponent::WinId TerminalMultiplexerComponent::GetWindowCount() const {
        return this->windows.size();
    }

    bool TerminalMultiplexerComponent::SetFocus(WinId idx) {
        if (idx <= this->windows.size()) {
            this->focus = idx;
            return true;
        } else {
            return false;
        }
    }

    SlokedIndexed<SlokedComponentHandle &, TerminalMultiplexerComponent::WinId> TerminalMultiplexerComponent::NewWindow(const TextPosition &pos, const TextPosition &dim) {
        auto terminal = std::make_shared<TerminalWindow>(this->terminal, this->encoding, this->charWidth, pos.column, pos.line, dim.column, dim.line, [](const TerminalWindow &) {
            return std::vector<SlokedKeyboardInput>{};
        });
        auto handle = std::make_shared<TerminalComponentHandle>(*terminal, this->encoding, this->charWidth);
        this->windows.push_back(std::make_pair(handle, terminal));
        return {this->windows.size() - 1, *handle};
    }

    bool TerminalMultiplexerComponent::CloseWindow(WinId idx) {
        if (idx < this->windows.size()) {
            this->windows.erase(this->windows.begin() + idx);
            return true;
        } else {
            return false;
        }
    }
    
    void TerminalMultiplexerComponent::ProcessInput(const SlokedKeyboardInput &input) {
        bool res = false;
        if (this->inputHandler) {
            res = this->inputHandler(input);
        }
        if (!res && this->focus < this->windows.size()) {
            this->windows.at(this->focus).first->ProcessInput(input);
        }
    }

    void TerminalMultiplexerComponent::Render() {
        for (auto &win : this->windows) {
            win.first->Render();
        }
    }
}