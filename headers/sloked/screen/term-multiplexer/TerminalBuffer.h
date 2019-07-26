#ifndef SLOCKED_SCREEN_TERM_MULTIPLEXER_H_
#define SLOCKED_SCREEN_TERM_MULTIPLEXER_H_

#include "sloked/core/Encoding.h"
#include "sloked/screen/Terminal.h"
#include "sloked/screen/term-multiplexer/Graphics.h"
#include <memory>

namespace sloked {

    class BufferedTerminal : public SlokedTerminal {
     public:
        BufferedTerminal(SlokedTerminal &, const Encoding &);
        virtual ~BufferedTerminal();

        void Flush();
        void UpdateSize();
    
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

        void SetGraphicsMode(SlokedTerminalText) override;
        void SetGraphicsMode(SlokedTerminalBackground) override;
        void SetGraphicsMode(SlokedTerminalForeground) override;
     private:
        void reset();

        struct Grapheme {
            bool updated = false;
            std::unique_ptr<BufferedGraphicsMode> graphics;
            std::string value;
        };

        SlokedTerminal &term;
        const Encoding &encoding;
        bool cls;
        bool show_cursor;
        Grapheme *buffer;
        std::unique_ptr<BufferedGraphicsMode> graphics;
        Line line;
        Column col;
        Column width;
        Line height;
    };
}

#endif