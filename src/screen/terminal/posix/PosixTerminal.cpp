/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/screen/terminal/posix/PosixTerminal.h"
#include <optional>
#include <map>
#include <array>
#include <cassert>
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
        { "k9", SlokedControlKey::F9 },
        { "k;", SlokedControlKey::F10 },
        { "F1", SlokedControlKey::F11 },
        { "F2", SlokedControlKey::F12 }
    };

    static const std::map<std::string, std::string> CompatConversions = {
        { "\033OA", "\033[A" },
        { "\033OB", "\033[B" },
        { "\033OC", "\033[C" },
        { "\033OD", "\033[D" },
        { "\033OH", "\033[H" }
    };
    
    static const std::map<char, SlokedControlKey> ControlKeys = {
        { '\000', SlokedControlKey::CtrlSpace },
        { '\001', SlokedControlKey::CtrlA },
        { '\002', SlokedControlKey::CtrlB },
        { '\004', SlokedControlKey::CtrlD },
        { '\005', SlokedControlKey::CtrlE },
        { '\006', SlokedControlKey::CtrlF },
        { '\007', SlokedControlKey::CtrlG },
        { '\010', SlokedControlKey::CtrlH },
        { '\011', SlokedControlKey::CtrlI },
        { '\012', SlokedControlKey::CtrlJ },
        { '\013', SlokedControlKey::CtrlK },
        { '\014', SlokedControlKey::CtrlL },
        { '\016', SlokedControlKey::CtrlN },
        { '\017', SlokedControlKey::CtrlO },
        { '\020', SlokedControlKey::CtrlP },
        { '\022', SlokedControlKey::CtrlR },
        { '\024', SlokedControlKey::CtrlT },
        { '\025', SlokedControlKey::CtrlU },
        { '\026', SlokedControlKey::CtrlV },
        { '\027', SlokedControlKey::CtrlW },
        { '\030', SlokedControlKey::CtrlX },
        { '\031', SlokedControlKey::CtrlY },
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
        for (const auto &specialKey : TermcapSpecialKeys) {
            const char *str = termcap.GetString(specialKey.first);
            if (str == nullptr) {
                continue;
            }
            std::string value{str};
            if (starts_with(input, value)) {
                input.erase(0, value.size());
                return specialKey.second;
            } else if (CompatConversions.count(value) && starts_with(input, CompatConversions.at(value))) {
                input.erase(0, CompatConversions.at(value).size());
                return specialKey.second;
            }
        }

        if (ControlKeys.count(input[0]) != 0) {
            char chr = input[0];
            input.erase(0, 1);
            return ControlKeys.at(chr);
        }
        
        switch (input[0]) {
            case '\b':
            case 127:
                input.erase(0, 1);
                return SlokedControlKey::Backspace;
                
            case '\t':
                input.erase(0, 1);
                return SlokedControlKey::Tab;

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
          termcap(std::make_unique<Termcap>(std::string(getenv("TERM")))),
          disable_flush(false),
          width(0), height(0) {
        fprintf(this->state->fd, this->termcap->GetString("ti"));
        fflush(this->state->fd);
        this->Update();
    }

    PosixTerminal::~PosixTerminal() {
        fprintf(this->state->fd, this->termcap->GetString("te"));
        fflush(this->state->fd);
    }

    void PosixTerminal::SetPosition(Line l, Column c) {
        if (!this->disable_flush) {
            fprintf(this->state->fd, "%s", tgoto(this->termcap->GetString("cm"), c, l));
            fflush(this->state->fd);
        } else {
            this->buffer << tgoto(this->termcap->GetString("cm"), c, l);
        }
    }

    void PosixTerminal::MoveUp(Line l) {
        if (!this->disable_flush) {
            while (l--) {
                fprintf(this->state->fd, this->termcap->GetString("up"));
            }
            fflush(this->state->fd);
        } else {
            while (l--) {
                this->buffer << this->termcap->GetString("up");
            }
        }
    }

    void PosixTerminal::MoveDown(Line l) {
        if (!this->disable_flush) {
            while (l--) {
                fprintf(this->state->fd, this->termcap->GetString("do"));
            }
            fflush(this->state->fd);
        } else {
            while (l--) {
                this->buffer << this->termcap->GetString("do");
            }
        }
    }

    void PosixTerminal::MoveBackward(Column c) {
        if (!this->disable_flush) {
            while (c--) {
                fprintf(this->state->fd, this->termcap->GetString("le"));
            }
            fflush(this->state->fd);
        } else {
            while (c--) {
                this->buffer << this->termcap->GetString("le");
            }
        }
    }

    void PosixTerminal::MoveForward(Column c) {
        if (!this->disable_flush) {
            while (c--) {
                fprintf(this->state->fd, this->termcap->GetString("ri"));
            }
            fflush(this->state->fd);
        } else {
            while (c--) {
                this->buffer << this->termcap->GetString("ri");
            }
        }
    }

    void PosixTerminal::ShowCursor(bool show) {
        if (!this->disable_flush) {
            if (show) {
                fprintf(this->state->fd, this->termcap->GetString("ve"));
            } else {
                fprintf(this->state->fd, this->termcap->GetString("vi"));
            }
            fflush(this->state->fd);
        } else {
            if (show) {
                this->buffer << this->termcap->GetString("ve");
            } else {
                this->buffer << this->termcap->GetString("vi");
            }
        }
    }

    void PosixTerminal::ClearScreen() {
        if (!this->disable_flush) {
            fprintf(this->state->fd, this->termcap->GetString("cl"));
            fflush(this->state->fd);
        } else {
            this->buffer << this->termcap->GetString("cl");
        }
    }

    void PosixTerminal::ClearChars(Column cols) {
        std::string cl(cols, ' ');
        if (!this->disable_flush) {
            fprintf(this->state->fd, cl.c_str());
            fflush(this->state->fd);
        } else {
            this->buffer << cl;
        }
        this->MoveBackward(cols);
    }

    TextPosition::Column PosixTerminal::GetWidth() {
        return this->width;
    }

    TextPosition::Line PosixTerminal::GetHeight() {
        return this->height;
    }

    void PosixTerminal::Write(const std::string &str) {
        if (!this->disable_flush) {
            fprintf(this->state->fd, "%s", str.c_str());
            fflush(this->state->fd);
        } else {
            this->buffer << str;
        }
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
    }    

    void PosixTerminal::SetGraphicsMode(SlokedTextGraphics mode) {
        unsigned int imode = 0;
        switch (mode) {
            case SlokedTextGraphics::Off:
                imode = 0;
                break;

            case SlokedTextGraphics::Bold:
                imode =  1;
                break;

            case SlokedTextGraphics::Underscore:
                imode = 4;
                break;

            case SlokedTextGraphics::Blink:
                imode = 5;
                break;

            case SlokedTextGraphics::Reverse:
                imode =  7;
                break;

            case SlokedTextGraphics::Concealed:
                imode = 8;
                break;
            
            default:
                assert(false);
                break;
        }
        if (!this->disable_flush) {
            fprintf(this->state->fd, "\033[%um", static_cast<unsigned int>(imode));
        } else {
            this->buffer << "\033[" << imode << "m";
        }
    }

    void PosixTerminal::SetGraphicsMode(SlokedBackgroundGraphics mode) {
        if (!this->disable_flush) {
            fprintf(this->state->fd, "\033[%um", static_cast<unsigned int>(mode) + 40);
        } else {
            this->buffer << "\033[" << static_cast<unsigned int>(mode) + 40 << "m";
        }
    }

    void PosixTerminal::SetGraphicsMode(SlokedForegroundGraphics mode) {
        if (!this->disable_flush) {
            fprintf(this->state->fd, "\033[%um", static_cast<unsigned int>(mode) + 30);
        } else {
            this->buffer << "\033[" << static_cast<unsigned int>(mode) + 30 << "m";
        }
    }

    void PosixTerminal::Update() {
        struct winsize w;
        ioctl(fileno(this->state->input), TIOCGWINSZ, &w);
        this->width = w.ws_col;
        this->height = w.ws_row;
    }

    void PosixTerminal::Flush(bool flush) {
        if (flush) {
            fprintf(this->state->fd, "%s", this->buffer.str().c_str());
            fflush(this->state->fd);
            this->buffer.clear();
            this->buffer.str(std::string());
            this->disable_flush = false;
        } else {
            this->disable_flush = true;
        }
    }
}