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
        TerminalWindow(SlokedTerminal &, const Encoding &, const SlokedCharWidth &, const TextPosition &, const TextPosition &);

        void Move(const TextPosition &);
        void Resize(const TextPosition &);
        const TextPosition &GetOffset() const;
        TerminalWindow SubWindow(const TextPosition &, const TextPosition &) const;

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

        void Update() override;
        void Flush(bool) override;

     private:
        SlokedTerminal &term;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        TextPosition offset;
        TextPosition size;
        Line line;
        Column col;
    };
}

#endif