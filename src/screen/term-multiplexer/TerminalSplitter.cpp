#include "sloked/screen/term-multiplexer/TerminalSplitter.h"
#include <cassert>

namespace sloked {

    TerminalSplitter::TerminalSplitter(SlokedTerminal &term, Direction dir, const Encoding &encoding)
        : term(term), direction(dir), encoding(encoding) {}

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

    SlokedTerminal *TerminalSplitter::GetWindow(std::size_t idx) const {
        if (idx < this->windows.size()) {
            return this->windows.at(idx).first.get();
        } else {
            return nullptr;
        }
    }

    std::size_t TerminalSplitter::GetWindowCount() const {
        return this->windows.size();
    }

    SlokedTerminal &TerminalSplitter::NewWindow(const Constraints &constraints) {
        auto win = std::make_shared<TerminalWindow>(this->term, this->encoding, 0, 0, 0, 0, [&]() {
            return this->term.GetInput();
        });
        this->windows.push_back(std::make_pair(win, constraints));
        this->Update();
        return *win;
    }

    void TerminalSplitter::Update() {
        unsigned int current = 0;
        unsigned int max = this->direction == Direction::Horizontal
            ? this->term.GetWidth()
            : this->term.GetHeight();
        unsigned int dim = this->direction == Direction::Horizontal
            ? this->term.GetHeight()
            : this->term.GetWidth();
        for (const auto &win : this->windows) {
            unsigned winDim = win.second.Calc(max);
            if (this->direction == Direction::Horizontal) {
                win.first->Move(current, 0);
                win.first->Resize(winDim, dim);
            } else {
                win.first->Move(0, current);
                win.first->Resize(dim, winDim);
            }
            current += winDim;
        }
        this->term.SetGraphicsMode(SlokedTerminalText::Off);
        if (current < max) {
            if (this->direction == Direction::Horizontal) {
                this->term.SetPosition(current, 0);
            } else {
                this->term.SetPosition(0, current);
            }
            for (unsigned int i = current; i < max; i++) {
                if (this->direction == Direction::Horizontal) {
                    this->term.ClearChars(max - current);
                } else {
                    this->term.ClearChars(dim);
                }
                this->term.MoveDown(1);
            }
        }
    }

    TerminalSplitter::Constraints::Constraints(float dim, unsigned int min, unsigned int max)
        : dim(dim), min(min), max(max) {
        assert(dim >= 0.0f);
        assert(dim <= 1.0f);
        assert(min <= max || max == 0);
    }

    float TerminalSplitter::Constraints::GetDimensions() const {
        return this->dim;
    }

    unsigned int TerminalSplitter::Constraints::GetMinimum() const {
        return this->min;
    }

    unsigned int TerminalSplitter::Constraints::GetMaximum() const {
        return this->max;
    }

    unsigned int TerminalSplitter::Constraints::Calc(unsigned int max) const {
        unsigned int dim = static_cast<unsigned int>(static_cast<float>(max) * this->dim);
        if (dim < this->min) {
            dim = this->min;
        }
        if (dim > this->max && this->max > 0) {
            dim = this->max;
        }
        return dim;
    }
}