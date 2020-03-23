/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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
#include "sloked/core/String.h"
#include "sloked/core/posix/Time.h"
#include <optional>
#include <map>
#include <array>
#include <cassert>
#include <termcap.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace sloked {

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
            if (cache.count(key)) {
                return cache.at(key).c_str();
            } else {
                char *data = this->buffer.data();
                auto result = tgetstr(key.c_str(), &data);
                cache.emplace(key, std::string{result});
                return result;
            }
        }

     private:
        std::array<char, 2048> buffer;
        std::map<std::string, std::string> cache;
    };

    std::optional<SlokedControlKey> collectControlKey(PosixTerminal::Termcap &termcap, std::string &input, bool &altPressed) {
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
                } else if (input.size() > 1) {
                    input.erase(0, 1);
                    altPressed = true;
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
        auto str = this->termcap->GetString("ti");
        if (str != nullptr) {
            fprintf(this->state->fd, "%s", str);
        }
        fflush(this->state->fd);
        this->UpdateDimensions();
    }

    PosixTerminal::~PosixTerminal() {
        auto str = this->termcap->GetString("te");
        if (str != nullptr) {
            fprintf(this->state->fd, "%s", str);
        }
        fflush(this->state->fd);
    }

    void PosixTerminal::SetPosition(Line l, Column c) {
        std::pair<Line, Column> base{l, c};
        const char *basePos;
        if (this->cursorMotionCache.count(base) > 0) {
            basePos = this->cursorMotionCache.at(base).c_str();
        } else {
            basePos = tgoto(this->termcap->GetString("cm"), c, l);
            this->cursorMotionCache.emplace(base, std::string(basePos));
        }
        if (!this->disable_flush) {
            fprintf(this->state->fd, "%s", basePos);
            fflush(this->state->fd);
        } else {
            this->buffer.append(basePos);
        }
    }

    void PosixTerminal::MoveUp(Line l) {
        if (!this->disable_flush) {
            while (l--) {
                fprintf(this->state->fd, "%s", this->termcap->GetString("up"));
            }
            fflush(this->state->fd);
        } else {
            while (l--) {
                this->buffer.append(this->termcap->GetString("up"));
            }
        }
    }

    void PosixTerminal::MoveDown(Line l) {
        if (!this->disable_flush) {
            while (l--) {
                fprintf(this->state->fd, "%s", this->termcap->GetString("do"));
            }
            fflush(this->state->fd);
        } else {
            while (l--) {
                this->buffer.append(this->termcap->GetString("do"));
            }
        }
    }

    void PosixTerminal::MoveBackward(Column c) {
        if (!this->disable_flush) {
            while (c--) {
                fprintf(this->state->fd, "%s", this->termcap->GetString("le"));
            }
            fflush(this->state->fd);
        } else {
            while (c--) {
                this->buffer.append(this->termcap->GetString("le"));
            }
        }
    }

    void PosixTerminal::MoveForward(Column c) {
        if (!this->disable_flush) {
            while (c--) {
                fprintf(this->state->fd, "%s", this->termcap->GetString("ri"));
            }
            fflush(this->state->fd);
        } else {
            while (c--) {
                this->buffer.append(this->termcap->GetString("ri"));
            }
        }
    }

    void PosixTerminal::ShowCursor(bool show) {
        if (!this->disable_flush) {
            if (show) {
                fprintf(this->state->fd, "%s", this->termcap->GetString("ve"));
            } else {
                fprintf(this->state->fd, "%s", this->termcap->GetString("vi"));
            }
            fflush(this->state->fd);
        } else {
            if (show) {
                this->buffer.append(this->termcap->GetString("ve"));
            } else {
                this->buffer.append(this->termcap->GetString("vi"));
            }
        }
    }

    void PosixTerminal::ClearScreen() {
        if (!this->disable_flush) {
            fprintf(this->state->fd, "%s", this->termcap->GetString("cl"));
            fflush(this->state->fd);
        } else {
            this->buffer.append(this->termcap->GetString("cl"));
        }
    }

    void PosixTerminal::ClearChars(Column cols) {
        while (cols--) {
            if (!this->disable_flush) {
                fprintf(this->state->fd, " ");
                fflush(this->state->fd);
            } else {
                this->buffer.push_back(' ');
            }
        }
        this->MoveBackward(cols);
    }

    TextPosition::Column PosixTerminal::GetWidth() {
        return this->width;
    }

    TextPosition::Line PosixTerminal::GetHeight() {
        return this->height;
    }

    void PosixTerminal::Write(std::string_view str) {
        if (!this->disable_flush) {
            fprintf(this->state->fd, "%*s", static_cast<int>(str.size()), str.data());
            fflush(this->state->fd);
        } else {
            this->buffer.append(str);
        }
    }

    FILE *PosixTerminal::GetOutputFile() {
        return this->state->fd;
    }

    bool PosixTerminal::WaitInput(std::chrono::system_clock::duration timeout) {
        fd_set rfds;
        struct timeval tv;
        FD_ZERO(&rfds);
        auto fno = fileno(this->state->input);
        FD_SET(fno, &rfds);
        DurationToTimeval(timeout, tv);

        return select(fno + 1, &rfds, NULL, NULL, &tv) > 0;
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
        bool altPressed = false;
        while (!view.empty()) {
            bool prevAlt = altPressed;
            auto specialKey = collectControlKey(*this->termcap, view, altPressed);
            if (!prevAlt && altPressed) { // Alt keycode is detected skip to get the next input chunk
                continue;
            }
            if (specialKey.has_value()) {
                if (!buf.empty()) {
                    input.push_back(SlokedKeyboardInput{buf, false});
                    buf.clear();
                }
                input.push_back(SlokedKeyboardInput{specialKey.value(), altPressed});
            } else if (!view.empty()) {
                buf.push_back(view[0]);
                view.erase(0, 1);
            }
            altPressed = false;
        }
        if (!buf.empty()) {
            input.push_back(SlokedKeyboardInput{buf, false});
        }
        return input;
    }

    void PosixTerminal::SetGraphicsMode(SlokedTextGraphics mode) {
        constexpr auto GraphicModeCount = static_cast<int>(SlokedTextGraphics::Concealed) + 1;
        static auto GraphicsModes = []() {
            std::array<std::string, GraphicModeCount> modes;
            for (std::size_t i = 0; i < modes.size(); i++) {
                modes[i] = std::to_string(i);
            }
            return modes;
        }();
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
        }
        if (!this->disable_flush) {
            fprintf(this->state->fd, "\033[%um", static_cast<unsigned int>(imode));
        } else {
            this->buffer.append("\033[");
            this->buffer.append(GraphicsModes[imode]);
            this->buffer.push_back('m');
        }
    }

    void PosixTerminal::SetGraphicsMode(SlokedBackgroundGraphics mode) {
        constexpr auto GraphicModeOffset = 40;
        constexpr auto GraphicModeCount = static_cast<int>(SlokedBackgroundGraphics::White) + 1;
        static auto GraphicsModes = []() {
            std::array<std::string, GraphicModeCount> modes;
            for (std::size_t i = 0; i < modes.size(); i++) {
                modes[i] = std::to_string(i + GraphicModeOffset);
            }
            return modes;
        }();
        if (!this->disable_flush) {
            fprintf(this->state->fd, "\033[%um", static_cast<unsigned int>(mode) + GraphicModeOffset);
        } else {
            this->buffer.append("\033[");
            this->buffer.append(GraphicsModes[static_cast<unsigned int>(mode)]);
            this->buffer.push_back('m');
        }
    }

    void PosixTerminal::SetGraphicsMode(SlokedForegroundGraphics mode) {
        constexpr auto GraphicModeOffset = 30;
        constexpr auto GraphicModeCount = static_cast<int>(SlokedForegroundGraphics::White) + 1;
        static auto GraphicsModes = []() {
            std::array<std::string, GraphicModeCount> modes;
            for (std::size_t i = 0; i < modes.size(); i++) {
                modes[i] = std::to_string(i + GraphicModeOffset);
            }
            return modes;
        }();
        if (!this->disable_flush) {
            fprintf(this->state->fd, "\033[%um", static_cast<unsigned int>(mode) + GraphicModeOffset);
        } else {
            this->buffer.append("\033[");
            this->buffer.append(GraphicsModes[static_cast<unsigned int>(mode)]);
            this->buffer.push_back('m');
        }
    }

    bool PosixTerminal::UpdateDimensions() {
        struct winsize w;
        ioctl(fileno(this->state->input), TIOCGWINSZ, &w);
        bool changed = this->width != w.ws_col || this->height != w.ws_row;
        this->width = w.ws_col;
        this->height = w.ws_row;
        return changed;
    }

    void PosixTerminal::Flush(bool flush) {
        if (flush) {
            fprintf(this->state->fd, "%s", this->buffer.c_str());
            fflush(this->state->fd);
            this->buffer.clear();
            this->disable_flush = false;
        } else {
            this->disable_flush = true;
        }
    }
}