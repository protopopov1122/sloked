#include "sloked/core/Error.h"
#include "sloked/screen/terminal/components/MultiplexerComponent.h"

namespace sloked {

    TerminalMultiplexerComponent::TerminalMultiplexerWindow::TerminalMultiplexerWindow(
        Id id, std::unique_ptr<TerminalComponentHandle> component, std::unique_ptr<TerminalWindow> window, TerminalMultiplexerComponent &root)
        : id(id), component(std::move(component)), window(std::move(window)), root(root) {}
    
    TerminalMultiplexerComponent::TerminalMultiplexerWindow::~TerminalMultiplexerWindow() {
        if (this->IsOpen()) {
            this->Close();
        }
    }

    bool TerminalMultiplexerComponent::TerminalMultiplexerWindow::IsOpen() const {
        return this->root.windows.count(this->id) != 0;
    }

    bool TerminalMultiplexerComponent::TerminalMultiplexerWindow::HasFocus() const {
        return this->root.focus.back() == this->id;
    }

    SlokedComponentHandle &TerminalMultiplexerComponent::TerminalMultiplexerWindow::GetComponent() const {
        if (this->component) {
            return *this->component;
        } else {
            throw SlokedError("Window already closed");
        }
    }

    TerminalMultiplexerComponent::TerminalMultiplexerWindow::Window::Id TerminalMultiplexerComponent::TerminalMultiplexerWindow::GetId() const {
        return this->id;
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::SetFocus() {
        if (this->IsOpen()) {
            std::remove(this->root.focus.begin(), this->root.focus.end(), this->id);
            this->root.focus.push_back(this->id);
        } else {
            throw SlokedError("Window already closed");
        }
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::Move(const TextPosition &pos) {
        if (this->window) {
            this->window->SetPosition(pos.line, pos.column);
        } else {
            throw SlokedError("Window already closed");
        }
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::Resize(const TextPosition &pos) {
        if (this->window) {
            this->window->Resize(pos);
        } else {
            throw SlokedError("Window already closed");
        }
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::Close() {
        if (this->IsOpen()) {
            std::remove(this->root.focus.begin(), this->root.focus.end(), this->id);
            this->component.reset();
            this->window.reset();
            this->root.windows.erase(this->id);
        } else {
            throw SlokedError("Window already closed");
        }
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::Render() {
        if (this->component) {
            this->component->Render();
        }
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::Update() {
        if (this->component) {
            this->component->UpdateDimensions();
        }
    }

    void TerminalMultiplexerComponent::TerminalMultiplexerWindow::ProcessInput(const SlokedKeyboardInput &input) {
        if (this->component) {
            this->component->ProcessInput(input);
        }
    }

    TerminalMultiplexerComponent::TerminalMultiplexerComponent(SlokedTerminal &term, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : terminal(term), encoding(encoding), charWidth(charWidth), focus(0), nextId(0) {}

    std::shared_ptr<TerminalMultiplexerComponent::Window> TerminalMultiplexerComponent::GetFocus() const {
        if (!this->focus.empty()) {
            return this->windows.at(this->focus.back());
        } else {
            return nullptr;
        }
    }

    std::shared_ptr<TerminalMultiplexerComponent::Window> TerminalMultiplexerComponent::GetWindow(Window::Id id) const {
        if (this->windows.count(id)) {
            return this->windows.at(id);
        } else {
            return nullptr;
        }
    }

    std::size_t TerminalMultiplexerComponent::GetWindowCount() const {
        return this->windows.size();
    }

    std::shared_ptr<TerminalMultiplexerComponent::Window> TerminalMultiplexerComponent::NewWindow(const TextPosition &pos, const TextPosition &dim) {
        auto terminal = std::make_unique<TerminalWindow>(this->terminal, this->encoding, this->charWidth, pos, dim);
        auto handle = std::make_unique<TerminalComponentHandle>(*terminal, this->encoding, this->charWidth);
        auto id = this->nextId++;
        auto window = std::make_shared<TerminalMultiplexerWindow>(id, std::move(handle), std::move(terminal), *this);
        this->windows[id] = window;
        this->focus.push_back(id);
        return window;
    }

    void TerminalMultiplexerComponent::Render() {
        for (auto id : this->focus) {
            this->windows.at(id)->Render();
        }
    }

    void TerminalMultiplexerComponent::UpdateDimensions() {
        for (auto kv : this->windows) {
            kv.second->Update();
        }
    }

    void TerminalMultiplexerComponent::ProcessComponentInput(const SlokedKeyboardInput &input) {
        if (!this->focus.empty()) {
            this->windows.at(this->focus.back())->ProcessInput(input);
        }
    }
}