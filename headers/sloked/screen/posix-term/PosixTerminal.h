#ifndef SLOKED_SCREEN_POSIXTERM_ANSICONSOLE_H_
#define SLOKED_SCREEN_POSIXTERM_ANSICONSOLE_H_

#include "sloked/Base.h"
#include <cstdio>
#include <string>
#include <variant>
#include <vector>
#include <memory>
#include <array>

namespace sloked {

    template <typename C, typename ... T>
    struct AnsiGraphicsMode;

    template <typename C, typename T>
    struct AnsiGraphicsMode<C, T> {
        static void Set(FILE *fd, T mode) {
            fprintf(fd, "%um", static_cast<unsigned int>(mode));
        }
    };

    template <typename C, typename A, typename ... B>
    struct AnsiGraphicsMode<C, A, B...> {
        static void Set(FILE *fd, A mode, B ... modes) {
            fprintf(fd, "%um;", static_cast<unsigned int>(mode));
            AnsiGraphicsMode<C, B...>::Set(fd, modes...);
        }
    };

    enum class PosixTerminalKey {
        ArrowUp,
        ArrowDown,
        ArrowLeft,
        ArrowRight,
        Backspace,
        Enter,
        F1
    };

    using PosixTerminalInput = std::variant<std::string, PosixTerminalKey>;
    
    class PosixTerminal {
     public:
        using Line = unsigned int;
        using Column = unsigned int;
        class Termcap;

        enum class Text;
        enum class Foreground;
        enum class Background;

        PosixTerminal(FILE * = stdout, FILE * = stdin);
        ~PosixTerminal();
    
        void SetPosition(Line, Column);
        void MoveUp(Line);
        void MoveDown(Line);
        void MoveBackward(Column);
        void MoveForward(Column);

        void ClearScreen();
        void ClearLine();

        void Write(const std::string &);

        std::vector<PosixTerminalInput> GetInput();

        template <typename ... T>
        void SetGraphicsMode(T ... modes) {
            fprintf(this->GetOutputFile(), "\033[");
            AnsiGraphicsMode<PosixTerminal, T...>::Set(this->GetOutputFile(), modes...);
        }

     private:
        struct State;
        FILE *GetOutputFile();

        std::unique_ptr<State> state;
        std::unique_ptr<Termcap> termcap;
        std::array<char, 2048> buffer;
    };

    enum class PosixTerminal::Text {
        Off = 0,
        Bold = 1,
        Underscore = 4,
        Blink = 5,
        Reverse = 7,
        Concealed = 8
    };

    enum class PosixTerminal::Foreground {
        Black = 30,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White
    };

    enum class PosixTerminal::Background {
        Black = 40,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White
    };
}

#endif