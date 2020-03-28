#include "sloked/screen/graphics/Terminal.h"

namespace sloked {

    SlokedGraphicalTerminalWindow::Parameters::Parameters(SlokedGraphicsDimensions size, const std::string &font)
        : size{size}, font{font} {}

    SlokedGraphicalTerminalWindow::Parameters &SlokedGraphicalTerminalWindow::Parameters::Size(SlokedGraphicsDimensions size) {
        this->size = size;
        return *this;
    }

    SlokedGraphicalTerminalWindow::Parameters &SlokedGraphicalTerminalWindow::Parameters::Title(const std::string &title) {
        this->title = title;
        return *this;
    }

    SlokedGraphicalTerminalWindow::Parameters &SlokedGraphicalTerminalWindow::Parameters::Font(const std::string &font) {
        this->font = font;
        return *this;
    }

    SlokedGraphicalTerminalWindow::Parameters &SlokedGraphicalTerminalWindow::Parameters::DefaultMode(const SlokedGraphicalTerminal::Mode &mode) {
        this->defaultMode = mode;
        return *this;
    }
}