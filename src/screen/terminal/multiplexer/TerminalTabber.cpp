#include "sloked/screen/terminal/multiplexer/TerminalTabber.h"
#include <functional>

namespace sloked {

    class TerminalTab : public SlokedTerminal {
     public:
        TerminalTab(std::function<bool(const TerminalTab *)> is_current, SlokedTerminal &term)
            : is_current(is_current), term(term) {}

        void SetPosition(Line l, Column c) override {
            if (this->is_current(this)) {
                this->term.SetPosition(l, c);
            }
        }

        void MoveUp(Line l) override {
            if (this->is_current(this)) {
                this->term.MoveUp(l);
            }
        }

        void MoveDown(Line l) override {
            if (this->is_current(this)) {
                this->term.MoveDown(l);
            }
        }

        void MoveBackward(Column c) override {
            if (this->is_current(this)) {
                this->term.MoveBackward(c);
            }
        }

        void MoveForward(Column c) override {
            if (this->is_current(this)) {
                this->term.MoveForward(c);
            }
        }

        void ShowCursor(bool s) override {
            if (this->is_current(this)) {
                this->term.ShowCursor(s);
            }
        }

        void ClearScreen() override {
            if (this->is_current(this)) {
                this->term.ClearScreen();
            }
        }

        void ClearChars(Column c) override {
            if (this->is_current(this)) {
                this->term.ClearChars(c);
            }
        }

        Column GetWidth() override {
            return this->term.GetWidth();
        }

        Line GetHeight() override {
            return this->term.GetHeight();
        }

        void Write(const std::string &str) override {
            if (this->is_current(this)) {
                this->term.Write(str);
            }
        }

        std::vector<SlokedKeyboardInput> GetInput() override {
            if (this->is_current(this)) {
                return this->term.GetInput();
            } else {
                return {};
            }
        }

        void SetGraphicsMode(SlokedTextGraphics m) override {
            if (this->is_current(this)) {
                this->term.SetGraphicsMode(m);
            }
        }

        void SetGraphicsMode(SlokedBackgroundGraphics m) override {
            if (this->is_current(this)) {
                this->term.SetGraphicsMode(m);
            }
        }

        void SetGraphicsMode(SlokedForegroundGraphics m) override {
            if (this->is_current(this)) {
                this->term.SetGraphicsMode(m);
            }
        }

        void Update() override {
            this->term.Update();
        }

        void Flush(bool f) override {
            this->term.Flush(f);
        }

     private:
        std::function<bool(const TerminalTab *)> is_current;
        SlokedTerminal &term;
    };

    TerminalTabber::TerminalTabber(SlokedTerminal &term)
        : term(term), current_tab(0) {}

    SlokedTerminal &TerminalTabber::NewTab() {
        auto tab = std::make_shared<TerminalTab>([this](auto tabPtr) {
            if (this->current_tab < this->tabs.size()) {
                return tabPtr == this->tabs.at(this->current_tab).get();
            } else {
                return false;
            }
        }, this->term);
        this->tabs.push_back(tab);
        return *tab;
    }

    SlokedTerminal &TerminalTabber::NewTab(std::size_t idx) {
        auto tab = std::make_shared<TerminalTab>([this](auto tabPtr) {
            if (this->current_tab < this->tabs.size()) {
                return tabPtr == this->tabs.at(this->current_tab).get();
            } else {
                return false;
            }
        }, this->term);
        this->tabs.insert(this->tabs.begin() + idx, tab);
        return *tab;
    }

    void TerminalTabber::SelectTab(std::size_t tab) {
        this->current_tab = tab;
    }
    
    std::size_t TerminalTabber::GetCurrentTab() const {
        return this->current_tab;
    }

    SlokedTerminal *TerminalTabber::GetTab(std::size_t idx) const {
        if (idx < this->tabs.size()) {
            return this->tabs.at(idx).get();
        } else {
            return nullptr;
        }
    }
}