#ifndef SLOCKED_SCREEN_TERM_MULTIPLEXER_H_
#define SLOCKED_SCREEN_TERM_MULTIPLEXER_H_

#include "sloked/core/Encoding.h"
#include "sloked/screen/Terminal.h"
#include "sloked/screen/term-multiplexer/Graphics.h"
#include "sloked/screen/CharWidth.h"
#include <memory>
#include <optional>

namespace sloked {

    class BufferedTerminal : public SlokedTerminal {
     public:
        BufferedTerminal(SlokedTerminal &, const Encoding &, const ScreenCharWidth &);
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

        void Update() override;
        
     private:
        void dump_buffer(std::u32string_view, std::size_t);

        struct Character {
            bool updated = false;
            std::optional<BufferedGraphicsMode> graphics;
            char32_t value = '\0';
        };

        SlokedTerminal &term;
        const Encoding &encoding;
        const ScreenCharWidth &charWidth;
        bool cls;
        bool show_cursor;
        Character *buffer;
        BufferedGraphicsMode graphics;
        Line line;
        Column col;
        Column width;
        Line height;
    };
}

#endif