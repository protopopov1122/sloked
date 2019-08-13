#ifndef SLOKED_SCREEN_TERMINAL_SCREEN_COMPONENTHANDLE_H_
#define SLOKED_SCREEN_TERMINAL_SCREEN_COMPONENTHANDLE_H_

#include "sloked/core/Encoding.h"
#include "sloked/screen/widgets/ComponentHandle.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/CharWidth.h"
#include "sloked/screen/terminal/screen/TextPaneComponent.h"
#include <memory>

namespace sloked {

    class TerminalComponentHandle : public SlokedComponentHandle {
     public:
        TerminalComponentHandle(SlokedTerminal &, const Encoding &, const ScreenCharWidth &);

        void ProcessInput(const SlokedKeyboardInput &) override;
        void Render() override;
        SlokedTextPaneComponent &NewTextPane(std::unique_ptr<SlokedTextPaneWidget>) override;
        SlokedSplitterComponent &NewSplitter(Splitter::Direction) override;
        SlokedTabberComponent &NewTabber() override;

     private:
        SlokedTerminal &terminal;
        const Encoding &encoding;
        const ScreenCharWidth &charWidth;
        std::unique_ptr<SlokedScreenComponent> component;
    };
}

#endif