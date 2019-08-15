#ifndef SLOKED_SCREEN_TERMINAL_H_
#define SLOKED_SCREEN_TERMINAL_H_

#include "sloked/Base.h"
#include "sloked/core/Position.h"
#include "sloked/screen/Keyboard.h"
#include "sloked/screen/Graphics.h"
#include <vector>

namespace sloked {

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

    class SlokedTerminalInputSource {
     public:
        virtual ~SlokedTerminalInputSource() = default;
        virtual std::vector<SlokedKeyboardInput> GetInput() = 0;
    };

    class SlokedTerminal {
     public:
        using Line = TextPosition::Line;
        using Column = TextPosition::Column;
        using Text = SlokedTextGraphics;
        using Foreground = SlokedForegroundGraphics;
        using Background = SlokedBackgroundGraphics;

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

        virtual void SetGraphicsMode(SlokedTextGraphics) = 0;
        virtual void SetGraphicsMode(SlokedBackgroundGraphics) = 0;
        virtual void SetGraphicsMode(SlokedForegroundGraphics) = 0;

        virtual void Update() {}
        virtual void Flush(bool) {}
    };

    class SlokedDuplexTerminal : public SlokedTerminal, public SlokedTerminalInputSource {};
}

#endif