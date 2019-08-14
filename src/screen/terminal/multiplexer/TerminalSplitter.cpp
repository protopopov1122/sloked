#include "sloked/core/Error.h"
#include "sloked/screen/terminal/multiplexer/TerminalSplitter.h"
#include <cassert>
#include <iostream>

namespace sloked {

    TerminalSplitter::TerminalSplitter(SlokedTerminal &term, Splitter::Direction dir, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : term(term), direction(dir), encoding(encoding), charWidth(charWidth), focus(0) {}

    std::optional<TerminalSplitter::WinId> TerminalSplitter::GetFocus() const {
        if (this->focus < this->windows.size()) {
            return this->focus;
        } else {
            return std::optional<TerminalSplitter::WinId>{};
        }
    }

    unsigned int TerminalSplitter::GetMinimum() const {
        unsigned int min = 0;
        for (const auto &win : this->windows) {
            min += win.second.GetMinimum();
        }
        return min;
    }

    unsigned int TerminalSplitter::GetMaximum() const {
        unsigned int max = 0;
        for (const auto &win : this->windows) {
            max += win.second.GetMaximum();
        }
        return max;
    }

    SlokedTerminal &TerminalSplitter::GetTerminal(WinId idx) const {
        if (idx < this->windows.size()) {
            return *this->windows.at(idx).first;
        } else {
            throw SlokedError("Window #" + std::to_string(idx) + " not found");
        }
    }

    TerminalSplitter::WinId TerminalSplitter::GetTerminalCount() const {
        return this->windows.size();
    }

    bool TerminalSplitter::SetFocus(WinId focus) {
        if (focus <= this->windows.size()) {
            this->focus = focus;
            return true;
        } else {
            return false;
        }
    }

    SlokedIndexed<SlokedTerminal &, TerminalSplitter::WinId> TerminalSplitter::NewTerminal(const Splitter::Constraints &constraints) {
        auto win = std::make_shared<TerminalWindow>(this->term, this->encoding, charWidth, 0, 0, 0, 0, [this](const auto &win) {
            if (this->focus < this->windows.size() &&
                &win == this->windows.at(this->focus).first.get()) {
                return this->term.GetInput();
            } else {
                return std::vector<SlokedKeyboardInput> {};
            }
        });
        this->windows.push_back(std::make_pair(win, constraints));
        this->Update();
        return {this->windows.size() - 1, *win};
    }

    SlokedIndexed<SlokedTerminal &, TerminalSplitter::WinId> TerminalSplitter::NewTerminal(WinId idx, const Splitter::Constraints &constraints) {
        if (idx > this->windows.size()) {
            throw SlokedError("Incorrect window index " + std::to_string(idx));
        }
        auto win = std::make_shared<TerminalWindow>(this->term, this->encoding, charWidth, 0, 0, 0, 0, [this](const auto &win) {
            if (this->focus < this->windows.size() &&
                &win == this->windows.at(this->focus).first.get()) {
                return this->term.GetInput();
            } else {
                return std::vector<SlokedKeyboardInput> {};
            }
        });
        this->windows.insert(this->windows.begin() + idx, std::make_pair(win, constraints));
        this->Update();
        return {idx, *win};
    }

    bool TerminalSplitter::CloseTerminal(WinId idx) {
        if (idx < this->windows.size()) {
            this->windows.erase(this->windows.begin() + idx);
            return true;
        } else {
            return false;
        }
    }

    void TerminalSplitter::Update() {
        this->term.Update();
        unsigned int current = 0;
        unsigned int max = this->direction == Splitter::Direction::Horizontal
            ? this->term.GetWidth()
            : this->term.GetHeight();
        unsigned int dim = this->direction == Splitter::Direction::Horizontal
            ? this->term.GetHeight()
            : this->term.GetWidth();
        for (const auto &win : this->windows) {
            unsigned winDim = win.second.Calc(max);
            if (this->direction == Splitter::Direction::Horizontal) {
                win.first->Move(current, 0);
                win.first->Resize(winDim, dim);
            } else {
                win.first->Move(0, current);
                win.first->Resize(dim, winDim);
            }
            current += winDim;
        }
        this->term.SetGraphicsMode(SlokedTextGraphics::Off);
        if (current < max) {
            if (this->direction == Splitter::Direction::Horizontal) {
                this->term.SetPosition(current, 0);
            } else {
                this->term.SetPosition(0, current);
            }
            for (unsigned int i = current; i < max; i++) {
                if (this->direction == Splitter::Direction::Horizontal) {
                    this->term.ClearChars(max - current);
                } else {
                    this->term.ClearChars(dim);
                }
                this->term.MoveDown(1);
            }
        }
    }
}