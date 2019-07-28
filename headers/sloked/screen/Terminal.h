#ifndef SLOKED_SCREEN_TERMINAL_H_
#define SLOKED_SCREEN_TERMINAL_H_

#include "sloked/Base.h"
#include "sloked/screen/Keyboard.h"
#include <vector>

namespace sloked {

    enum class SlokedTerminalText {
        Off = 0,
        Bold,
        Underscore,
        Blink,
        Reverse,
        Concealed,
        Count
    };

    enum class SlokedTerminalForeground {
        Black = 0,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White
    };

    enum class SlokedTerminalBackground {
        Black = 0,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White
    };

    template <typename T, typename ... M>
    struct SlokedTerminalSetGraphicsMode {
        static void Set(T &) {}
    };

    template <typename T, typename A, typename ... M>
    struct SlokedTerminalSetGraphicsMode<T, A, M...> {
        static void Set(T &terminal, A mode, M... modes) {
            terminal.SetGraphicsMode(mode);
            SlokedTerminalSetGraphicsMode<T, M...>::Set(terminal, modes...);
        }
    };

    class SlokedTerminal {
     public:
        using Line = unsigned int;
        using Column = unsigned int;
        using Text = SlokedTerminalText;
        using Foreground = SlokedTerminalForeground;
        using Background = SlokedTerminalBackground;

        virtual ~SlokedTerminal() = default;
    
        virtual void SetPosition(Line, Column) = 0;
        virtual void MoveUp(Line) = 0;
        virtual void MoveDown(Line) = 0;
        virtual void MoveBackward(Column) = 0;
        virtual void MoveForward(Column) = 0;

        virtual void ShowCursor(bool) = 0;
        virtual void ClearScreen() = 0;
        virtual void ClearChars(Column) = 0;
        virtual Column GetWidth() = 0;
        virtual Line GetHeight() = 0;

        virtual void Write(const std::string &) = 0;
        virtual std::vector<SlokedKeyboardInput> GetInput() = 0;

        virtual void SetGraphicsMode(SlokedTerminalText) = 0;
        virtual void SetGraphicsMode(SlokedTerminalBackground) = 0;
        virtual void SetGraphicsMode(SlokedTerminalForeground) = 0;

        virtual void Update() {}
        virtual void Flush(bool) {}

        template <typename ... M>
        void SetGraphicsMode(M ... modes) {
            SlokedTerminalSetGraphicsMode<SlokedTerminal, M...>::Set(*this, modes...);
        }
    };
}

#endif