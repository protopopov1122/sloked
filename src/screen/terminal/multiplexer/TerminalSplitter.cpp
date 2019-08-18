#include "sloked/core/Error.h"
#include "sloked/screen/terminal/multiplexer/TerminalSplitter.h"
#include <cassert>
#include <iostream>

namespace sloked {

    class SplitterLayoutCalculator {
     public:
        SplitterLayoutCalculator(unsigned int max)
            : max(max), value(max), fraction(1.0f) {}

        unsigned int Next(const Splitter::Constraints &constraints) {
            unsigned int current = this->value * (constraints.GetDimensions() / this->fraction);
            if (current > constraints.GetMaximum() && constraints.GetMaximum() > 0) {
                current = constraints.GetMaximum();
            }
            if (current > this->value || (current == 0 && constraints.GetDimensions() > 0.0f)) {
                throw SlokedError("Layout error: insufficent space");
            }
            this->value -= current;
            this->fraction -= static_cast<float>(current) / this->max;
            return current;
        }

     private:
        unsigned int max;
        unsigned int value;
        float fraction;
    };

    TerminalSplitter::TerminalSplitter(SlokedTerminal &term, Splitter::Direction dir, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : term(term), direction(dir), encoding(encoding), charWidth(charWidth) {}

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

    const Splitter::Constraints &TerminalSplitter::GetConstraints(WinId idx) const {
        if (idx < this->windows.size()) {
            return this->windows.at(idx).second;
        } else {
            throw SlokedError("Incorrect window index " + std::to_string(idx));
        }
    }

    SlokedIndexed<SlokedTerminal &, TerminalSplitter::WinId> TerminalSplitter::NewTerminal(const Splitter::Constraints &constraints) {
        auto win = std::make_shared<TerminalWindow>(this->term, this->encoding, charWidth, 0, 0, 0, 0);
        this->windows.push_back(std::make_pair(win, constraints));
        this->Update();
        return {this->windows.size() - 1, *win};
    }

    SlokedIndexed<SlokedTerminal &, TerminalSplitter::WinId> TerminalSplitter::NewTerminal(WinId idx, const Splitter::Constraints &constraints) {
        if (idx > this->windows.size()) {
            throw SlokedError("Incorrect window index " + std::to_string(idx));
        }
        auto win = std::make_shared<TerminalWindow>(this->term, this->encoding, charWidth, 0, 0, 0, 0);
        this->windows.insert(this->windows.begin() + idx, std::make_pair(win, constraints));
        this->Update();
        return {idx, *win};
    }

    bool TerminalSplitter::UpdateConstraints(WinId idx, const Splitter::Constraints &constraints) {
        if (idx < this->windows.size()) {
            this->windows[idx].second = constraints;
            this->Update();
            return true;
        } else {
            return false;
        }
    }

    bool TerminalSplitter::Move(WinId src, WinId dst) {
        if (src < this->windows.size() && dst < this->windows.size()) {
            if (src < dst) {
                this->windows.insert(this->windows.begin() + dst + 1, this->windows.at(src));
                this->windows.erase(this->windows.begin() + src);
            } else if (src > dst) {
                this->windows.insert(this->windows.begin() + dst, this->windows.at(src));
                this->windows.erase(this->windows.begin() + src + 1);
            }
            return true;
        } else {
            return false;
        }
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
        unsigned int max = this->direction == Splitter::Direction::Horizontal
            ? this->term.GetWidth()
            : this->term.GetHeight();
        unsigned int dim = this->direction == Splitter::Direction::Horizontal
            ? this->term.GetHeight()
            : this->term.GetWidth();
        if (max < this->GetMinimum()) {
            throw SlokedError("Layout error: insufficent space");
        }
        SplitterLayoutCalculator calc(max - this->GetMinimum());
        unsigned int current = 0;
        for (const auto &win : this->windows) {
            unsigned winDim = calc.Next(win.second) + win.second.GetMinimum();
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