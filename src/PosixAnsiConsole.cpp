#include "sloked/PosixAnsiConsole.h"

namespace sloked {

    PosixAnsiConsole::PosixAnsiConsole(FILE *fd, FILE *input)
        : fd(fd), input(input) {
        tcgetattr(fileno(this->fd), &this->out_state);
        tcgetattr(fileno(this->input), &this->in_state);
        termios out_state {this->out_state}, in_state {this->in_state};
        out_state.c_lflag &= ~ECHO;
        in_state.c_lflag &= (~ICANON & ~ECHO);
        tcsetattr(fileno(this->fd), TCSANOW, &out_state);
        tcsetattr(fileno(this->input), TCSANOW, &in_state);
    }
    
    PosixAnsiConsole::~PosixAnsiConsole() {
        tcsetattr(fileno(this->fd), TCSANOW, &this->out_state);
        tcsetattr(fileno(this->input), TCSANOW, &this->in_state);
    }
    
    void PosixAnsiConsole::SetPosition(Line l, Column c) {
        fprintf(this->fd, "\033[%u;%uH", l, c);
    }

    void PosixAnsiConsole::MoveUp(Line l) {
        fprintf(this->fd, "\033[%uA", l);
    }

    void PosixAnsiConsole::MoveDown(Line l) {
        fprintf(this->fd, "\033[%uB", l);
    }

    void PosixAnsiConsole::MoveBackward(Column c) {
        fprintf(this->fd, "\033[%uC", c);
    }

    void PosixAnsiConsole::MoveForward(Column c) {
        fprintf(this->fd, "\033[%uD", c);
    }

    void PosixAnsiConsole::ClearScreen() {
        fprintf(this->fd, "\033[2J");
    }

    void PosixAnsiConsole::ClearLine() {
        fprintf(this->fd, "\033[K");
    }

    void PosixAnsiConsole::Write(const std::string &str, bool flush) {
        fprintf(this->fd, str.c_str());
        if (flush) {
            fflush(this->fd);
        }
    }

    void PosixAnsiConsole::Flush() {
        fflush(this->fd);
    }

    int PosixAnsiConsole::GetChar() {
        return fgetc(this->input);
    }
}