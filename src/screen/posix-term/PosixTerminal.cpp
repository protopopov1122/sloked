#include "sloked/screen/posix-term/PosixTerminal.h"
#include <termcap.h>
#include <termios.h>
#include <cstring>
#include <optional>
#include <sys/ioctl.h>
#include <stropts.h>
#include <iostream>
#include <map>
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

    static const std::map<std::string, PosixTerminalKey> TermcapSpecialKeys = {
        { "ku", PosixTerminalKey::ArrowUp },
        { "kd", PosixTerminalKey::ArrowDown },
        { "kr", PosixTerminalKey::ArrowRight },
        { "kl", PosixTerminalKey::ArrowLeft },
        { "kb", PosixTerminalKey::Backspace },
        { "k1", PosixTerminalKey::F1 }
    };

    static const std::map<std::string, std::string> CompatConversions = {
        { "\033[A", "\033OA" },
        { "\033[B", "\033OB" },
        { "\033[C", "\033OC" },
        { "\033[D", "\033OD" },
        { "\033[P", "\033OP" }
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

    std::optional<PosixTerminalKey> checkKey(PosixTerminal::Termcap &termcap, std::string &input) {
        for (const auto &compat : CompatConversions) {
            if (starts_with(input, compat.first)) {
                input.replace(0, compat.first.size(), compat.second);
            }
        }

        for (const auto &specialKey : TermcapSpecialKeys) {
            std::string_view view {termcap.GetString(specialKey.first)};
            if (starts_with(input, view)) {
                input.erase(0, view.size());
                return specialKey.second;
            }
        }
        
        switch (input[0]) {
            case '\b':
            case 127:
                input.erase(0, 1);
                return PosixTerminalKey::Backspace;
            
            case '\n':
                input.erase(0, 1);
                return PosixTerminalKey::Enter;
        }

        return std::optional<PosixTerminalKey>{};
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

    void PosixTerminal::Write(const std::string &str) {
        fprintf(this->state->fd, "%s", str.c_str());
        fflush(this->state->fd);
    }

    FILE *PosixTerminal::GetOutputFile() {
        return this->state->fd;
    }

    std::vector<PosixTerminalInput> PosixTerminal::GetInput() {
        std::size_t n = 0;
        
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

        std::vector<PosixTerminalInput> input;
        std::string buf;
        while (!view.empty()) {
            auto specialKey = checkKey(*this->termcap, view);
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
}