#include "sloked/screen/terminal/multiplexer/TerminalSplitter.h"
#include <cassert>
#include <iostream>

namespace sloked {

    TerminalSplitter::TerminalSplitter(SlokedTerminal &term, Splitter::Direction dir, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : term(term), direction(dir), encoding(encoding), charWidth(charWidth), focus(0) {}

    void TerminalSplitter::SetFocus(std::size_t focus) {
        this->focus = focus;
    }

    std::size_t TerminalSplitter::GetFocus() const {
        return this->focus;
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

    SlokedTerminal *TerminalSplitter::GetTerminal(std::size_t idx) const {
        if (idx < this->windows.size()) {
            return this->windows.at(idx).first.get();
        } else {
            return nullptr;
        }
    }

    std::size_t TerminalSplitter::GetTerminalCount() const {
        return this->windows.size();
    }

    SlokedTerminal &TerminalSplitter::NewTerminal(const Splitter::Constraints &constraints) {
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
        return *win;
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