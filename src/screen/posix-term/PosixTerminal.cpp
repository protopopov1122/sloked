#include "sloked/screen/posix-term/PosixTerminal.h"
#include <optional>
#include <array>
#include <termcap.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace sloked {

    bool starts_with(std::string_view s1, std::string_view s2) {
        if (s1.size() >= s2.size()) {
            for (std::size_t i = 0; i < s2.size(); i++) {
                if (s1[i] != s2[i]) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    }

    static const std::vector<std::pair<std::string, SlokedControlKey>> TermcapSpecialKeys = {
        { "ku", SlokedControlKey::ArrowUp },
        { "kd", SlokedControlKey::ArrowDown },
        { "kr", SlokedControlKey::ArrowRight },
        { "kl", SlokedControlKey::ArrowLeft },
        { "kb", SlokedControlKey::Backspace },
        { "kD", SlokedControlKey::Delete },
        { "kI", SlokedControlKey::Insert },
        { "kN", SlokedControlKey::PageDown },
        { "kP", SlokedControlKey::PageUp },
        { "kh", SlokedControlKey::Home },
        { "k1", SlokedControlKey::F1 },
        { "k2", SlokedControlKey::F2 },
        { "k3", SlokedControlKey::F3 },
        { "k4", SlokedControlKey::F4 },
        { "k5", SlokedControlKey::F5 },
        { "k6", SlokedControlKey::F6 },
        { "k7", SlokedControlKey::F7 },
        { "k8", SlokedControlKey::F8 },
        { "k9", SlokedControlKey::F9 }
    };

    static const std::vector<std::pair<std::string, std::string>> CompatConversions = {
        { "\033[A", "\033OA" },
        { "\033[B", "\033OB" },
        { "\033[C", "\033OC" },
        { "\033[D", "\033OD" },
        { "\033[H", "\033OH" }
    };

    struct PosixTerminal::State {
        State(FILE *fd, FILE *input)
            :  fd(fd), input(input) {
            tcgetattr(fileno(this->fd), &this->out_state);
            tcgetattr(fileno(this->input), &this->in_state);
            termios out_state {this->out_state}, in_state {this->in_state};
            out_state.c_lflag &= ~ECHO;
            in_state.c_lflag &= (~ICANON & ~ECHO);
            tcsetattr(fileno(this->fd), TCSANOW, &out_state);
            tcsetattr(fileno(this->input), TCSANOW, &in_state);
        }

        ~State() {
            tcsetattr(fileno(this->fd), TCSANOW, &this->out_state);
            tcsetattr(fileno(this->input), TCSANOW, &this->in_state);
        }

        FILE *fd;
        FILE *input;
        termios out_state;
        termios in_state;
    };

    class PosixTerminal::Termcap {
     public:
        Termcap(const std::string &termtype) {
            tgetent(this->buffer.data(), termtype.c_str());
        }

        const char *GetString(const std::string &key) {
            char *data = this->buffer.data();
            return tgetstr(key.c_str(), &data);
        }

     private:
        std::array<char, 2048> buffer;
    };

    std::optional<SlokedControlKey> collectControlKey(PosixTerminal::Termcap &termcap, std::string &input) {
        for (const auto &compat : CompatConversions) {
            if (starts_with(input, compat.first)) {
                input.replace(0, compat.first.size(), compat.second);
                break;
            }
        }

        for (const auto &specialKey : TermcapSpecialKeys) {
            const char *str = termcap.GetString(specialKey.first);
            if (str == nullptr) {
                continue;
            }
            std::string_view view {str};
            if (starts_with(input, view)) {
                input.erase(0, view.size());
                return specialKey.second;
            }
        }
        
        switch (input[0]) {
            case '\b':
            case 127:
                input.erase(0, 1);
                return SlokedControlKey::Backspace;
            
            case '\n':
                input.erase(0, 1);
                return SlokedControlKey::Enter;

            case '\033':
                if (starts_with(input, "\033[F")) {
                    input.erase(0, 3);
                    return SlokedControlKey::End;
                } else {
                    input.erase(0, 1);
                    return SlokedControlKey::Escape;
                }
        }

        return std::optional<SlokedControlKey>{};
    }

    PosixTerminal::PosixTerminal(FILE *fd, FILE *input)
        : state(std::make_unique<State>(fd, input)),
          termcap(std::make_unique<Termcap>(std::string(getenv("TERM")))) {
        fprintf(this->state->fd, this->termcap->GetString("ti"));
        fflush(this->state->fd);
    }

    PosixTerminal::~PosixTerminal() {
        fprintf(this->state->fd, this->termcap->GetString("te"));
        fflush(this->state->fd);
    }

    void PosixTerminal::SetPosition(Line l, Column c) {
        fprintf(this->state->fd, "%s", tgoto(this->termcap->GetString("cm"), c, l));
        fflush(this->state->fd);
    }

    void PosixTerminal::MoveUp(Line l) {
        fprintf(this->state->fd, this->termcap->GetString("UP"), l);
        fflush(this->state->fd);
    }

    void PosixTerminal::MoveDown(Line l) {
        fprintf(this->state->fd, this->termcap->GetString("DO"), l);
        fflush(this->state->fd);
    }

    void PosixTerminal::MoveBackward(Column c) {
        fprintf(this->state->fd, this->termcap->GetString("LE"), c);
        fflush(this->state->fd);
    }

    void PosixTerminal::MoveForward(Column c) {
        fprintf(this->state->fd, this->termcap->GetString("RI"), c);
        fflush(this->state->fd);
    }

    void PosixTerminal::ClearScreen() {
        fprintf(this->state->fd, this->termcap->GetString("cl"));
        fflush(this->state->fd);
    }

    void PosixTerminal::ClearLine() {
        fprintf(this->state->fd, this->termcap->GetString("cd"));
        fflush(this->state->fd);
    }

    unsigned int PosixTerminal::GetWidth() {
        struct winsize w;
        ioctl(fileno(this->state->input), TIOCGWINSZ, &w);
        return w.ws_col;
    }

    unsigned int PosixTerminal::GetHeight() {
        struct winsize w;
        ioctl(fileno(this->state->input), TIOCGWINSZ, &w);
        return w.ws_row;
    }

    void PosixTerminal::Write(const std::string &str) {
        fprintf(this->state->fd, "%s", str.c_str());
        fflush(this->state->fd);
    }

    FILE *PosixTerminal::GetOutputFile() {
        return this->state->fd;
    }

    std::vector<SlokedKeyboardInput> PosixTerminal::GetInput() {
        std::string view;

        fd_set rfds;
        struct timeval tv;
        FD_ZERO(&rfds);
        auto fno = fileno(this->state->input);
        FD_SET(fno, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        do {
            char c;
            read(fno, &c, 1);
            view.push_back(c);
        } while (select(fno + 1, &rfds, NULL, NULL, &tv));

        std::vector<SlokedKeyboardInput> input;
        std::string buf;
        while (!view.empty()) {
            auto specialKey = collectControlKey(*this->termcap, view);
            if (specialKey.has_value()) {
                if (!buf.empty()) {
                    input.push_back(buf);
                    buf.clear();
                }
                input.push_back(specialKey.value());
            } else {
                buf.push_back(view[0]);
                view.erase(0, 1);
            }
        }
        if (!buf.empty()) {
            input.push_back(buf);
        }
        return input;
    }

    void set_terminal_mode(FILE *fd, unsigned int mode) {
        fprintf(fd, "\033[%um", static_cast<unsigned int>(mode));
    }    

    void PosixTerminal::SetGraphicsMode(SlokedTerminalText mode) {
        unsigned int imode;
        switch (mode) {
            case SlokedTerminalText::Off:
                imode = 0;
                break;

            case SlokedTerminalText::Bold:
                imode =  1;
                break;

            case SlokedTerminalText::Underscore:
                imode = 4;
                break;

            case SlokedTerminalText::Blink:
                imode = 5;
                break;

            case SlokedTerminalText::Reverse:
                imode =  7;
                break;

            case SlokedTerminalText::Concealed:
                imode = 8;
                break;
        }
        set_terminal_mode(this->state->fd, imode);
    }

    void PosixTerminal::SetGraphicsMode(SlokedTerminalBackground mode) {
        set_terminal_mode(this->state->fd, static_cast<unsigned int>(mode) + 40);
    }

    void PosixTerminal::SetGraphicsMode(SlokedTerminalForeground mode) {
        set_terminal_mode(this->state->fd, static_cast<unsigned int>(mode) + 30);
    }
}