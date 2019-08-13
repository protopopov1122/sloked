#ifndef SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALWINDOW_H_
#define SLOKED_SCREEN_TERMINAL_MULTIPLEXER_TERMINALWINDOW_H_

#include "sloked/core/Encoding.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/core/CharWidth.h"
#include "sloked/screen/terminal/multiplexer/BufferedGraphics.h"
#include <functional>

namespace sloked {

    class TerminalWindow : public SlokedTerminal {
     public:
        using InputSource = std::function<std::vector<SlokedKeyboardInput>(const TerminalWindow &)>;
        TerminalWindow(SlokedTerminal &, const Encoding &, const SlokedCharWidth &, Column, Line, Column, Line, InputSource);

        void Move(Column, Line);
        void Resize(Column, Line);
        Column GetOffsetX() const;
        Line GetOffsetY() const;
        TerminalWindow SubWindow(Column, Line, Column, Line, InputSource) const;

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
        std::vector<SlokedKeyboardInput> GetInput() override;

        void SetGraphicsMode(SlokedTextGraphics) override;
        void SetGraphicsMode(SlokedBackgroundGraphics) override;
        void SetGraphicsMode(SlokedForegroundGraphics) override;

        void Update() override;
        void Flush(bool) override;

     private:
        SlokedTerminal &term;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        Column offset_x;
        Line offset_y;
        Column width;
        Line height;
        InputSource inputSource;

        Column col;
        Line line;
    };
}

#endif