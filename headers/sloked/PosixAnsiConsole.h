#ifndef SLOKED_ANSICONSOLE_H_
#define SLOKED_ANSICONSOLE_H_

#include "sloked/Base.h"
#include <cstdio>
#include <string>
#include <termios.h>

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
    
    class PosixAnsiConsole {
     public:
        using Line = unsigned int;
        using Column = unsigned int;

        enum class Text;
        enum class Foreground;
        enum class Background;

        PosixAnsiConsole(FILE * = stdout, FILE * = stdin);
        ~PosixAnsiConsole();
    
        void SetPosition(Line, Column);
        void MoveUp(Line);
        void MoveDown(Line);
        void MoveBackward(Column);
        void MoveForward(Column);

        void ClearScreen();
        void ClearLine();

        void Write(const std::string &, bool = false);
        void Flush();

        int GetChar();

        template <typename ... T>
        void SetGraphicsMode(T ... modes) {
            fprintf(this->fd, "\033[");
            AnsiGraphicsMode<PosixAnsiConsole, T...>::Set(this->fd, modes...);
        }
     private:
        FILE *fd;
        FILE *input;
        termios out_state;
        termios in_state;
    };

    enum class PosixAnsiConsole::Text {
        Off = 0,
        Bold = 1,
        Underscore = 4,
        Blink = 5,
        Reverse = 7,
        Concealed = 8
    };

    enum class PosixAnsiConsole::Foreground {
        Black = 30,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White
    };

    enum class PosixAnsiConsole::Background {
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