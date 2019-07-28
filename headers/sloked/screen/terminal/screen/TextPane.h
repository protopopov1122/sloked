#ifndef SLOKED_SCREEN_TERMINAL_SCREEN_TEXTPANE_H_
#define SLOKED_SCREEN_TERMINAL_SCREEN_TEXTPANE_H_

#include "sloked/Base.h"
#include "sloked/screen/widgets/TextPane.h"
#include "sloked/screen/terminal/Terminal.h"

namespace sloked {

    class TerminalTextPane : public SlokedTextPane {
     public:
        TerminalTextPane(SlokedTerminal &);
    
        void SetPosition(Line, Column) override;
        void MoveUp(Line) override;
        void MoveDown(Line) override;
        void MoveBackward(Column) override;
        void MoveForward(Column) override;

        void ShowCursor(bool) override;
        void ClearScreen() override;
        void ClearChars(Column) override;
        Column GetWidth() override;
        Line GetHeight() override;

        void Write(const std::string &) override;

        void SetGraphicsMode(SlokedTextGraphics) override;
        void SetGraphicsMode(SlokedBackgroundGraphics) override;
        void SetGraphicsMode(SlokedForegroundGraphics) override;
        
     private:
        SlokedTerminal &term;
    };
}

#endif