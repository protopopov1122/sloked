#include "sloked/screen/term-multiplexer/TerminalScreenView.h"
#include "sloked/screen/term-multiplexer/TerminalWindow.h"

namespace sloked {

    TerminalScreenView::TerminalScreenView(SlokedTerminal &term)
        : term(&term) {
        this->columns = term.GetWidth();
        this->lines = term.GetHeight();
    }

    TerminalScreenView::TerminalScreenView(SlokedTerminal &term, SlokedTerminal::Column cols, SlokedTerminal::Line lines)
        : term(&term), columns(cols), lines(lines) {}

    TerminalScreenView::TerminalScreenView(TerminalScreenView &&view) {
        this->term = view.term;
        this->columns = view.columns;
        this->lines = view.lines;
        view.reset();
    }

    TerminalScreenView &TerminalScreenView::operator=(TerminalScreenView &&view) {
        this->term = view.term;
        this->columns = view.columns;
        this->lines = view.lines;
        view.reset();
        return *this;
    }

    bool TerminalScreenView::Empty() const {
        return this->term == nullptr;
    }

    std::unique_ptr<SlokedTerminal> TerminalScreenView::NewTerminal(const Encoding &encoding, const ScreenCharWidth &charWidth) {
        if (this->Empty()) {
            return nullptr;
        }
        auto res = std::make_unique<TerminalWindow>(*this->term, encoding, charWidth, 0, 0, this->columns, this->lines, [term = this->term]() {
            return term->GetInput();
        });
        this->reset();
        return res;
    }

    void TerminalScreenView::reset() {
        this->term = nullptr;
        this->columns = 0;
        this->lines = 0;
    }
}