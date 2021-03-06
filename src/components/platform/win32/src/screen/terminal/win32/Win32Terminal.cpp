#include "sloked/screen/terminal/win32/Win32Terminal.h"
#include "sloked/core/Encoding.h"
#include <array>
#include <optional>

namespace sloked {

    static constexpr UINT Codepage = 65001;
    static const Encoding &Encoding = Encoding::Utf8;

    struct Win32Terminal::State {
        State(HANDLE out, HANDLE in)
            : output(out), input(in) {
            GetConsoleMode(this->input, &this->originalMode);
            this->oldCodepage = GetConsoleCP();
            this->oldOutputCodepage = GetConsoleOutputCP();
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(this->output, &csbi);
            this->originalTextAttr = csbi.wAttributes;
        }

        ~State() {
            CONSOLE_SCREEN_BUFFER_INFO info;
            GetConsoleScreenBufferInfo(this->output, &info);
            DWORD nbWritten;
            FillConsoleOutputCharacterA(this->output, ' ', info.dwSize.X * info.dwSize.Y, {0, 0}, &nbWritten);
            SetConsoleMode(this->input, this->originalMode);
            SetConsoleCP(this->oldCodepage);
            SetConsoleOutputCP(this->oldOutputCodepage);
            SetConsoleTextAttribute(this->output, this->originalTextAttr);
        }

        HANDLE output;
        HANDLE input;
        DWORD originalMode;
        UINT oldCodepage;
        UINT oldOutputCodepage;
        DWORD originalTextAttr;
        TextPosition terminalSize{0, 0};
        std::chrono::system_clock::time_point lastResize;
        std::vector<std::function<void()>> buffer;
    };

    Win32Terminal::Win32Terminal()
        : Win32Terminal(GetStdHandle(STD_OUTPUT_HANDLE), GetStdHandle(STD_INPUT_HANDLE)) {}

    Win32Terminal::~Win32Terminal() = default;

    Win32Terminal::Win32Terminal(HANDLE output, HANDLE input)
        : state(std::make_unique<State>(output, input)), disable_flush{false} {
        SetConsoleMode(this->state->input, 0);
        SetConsoleCP(Codepage);
        SetConsoleOutputCP(Codepage);
        this->UpdateDimensions();
        this->ClearScreen();
    }

    void Win32Terminal::SetPosition(Line line, Column column) {
        auto callback = [this, line, column] {
            COORD pos;
            pos.X = column;
            pos.Y = line;
            SetConsoleCursorPosition(this->state->output, pos);
        };
        if (this->disable_flush) {
            this->state->buffer.emplace_back(std::move(callback));
        } else {
            callback();
        }
    }

    void Win32Terminal::MoveUp(Line line) {
        auto callback = [this, line] {
            CONSOLE_SCREEN_BUFFER_INFO info;
            GetConsoleScreenBufferInfo(this->state->output, &info);
            info.dwCursorPosition.Y -= line;
            SetConsoleCursorPosition(this->state->output, info.dwCursorPosition);
        };
        if (this->disable_flush) {
            this->state->buffer.emplace_back(std::move(callback));
        } else {
            callback();
        }
    }

    void Win32Terminal::MoveDown(Line line) {
        auto callback = [this, line] {
            CONSOLE_SCREEN_BUFFER_INFO info;
            GetConsoleScreenBufferInfo(this->state->output, &info);
            info.dwCursorPosition.Y += line;
            SetConsoleCursorPosition(this->state->output, info.dwCursorPosition);
        };
        if (this->disable_flush) {
            this->state->buffer.emplace_back(std::move(callback));
        } else {
            callback();
        }
    }

    void Win32Terminal::MoveBackward(Column col) {
        auto callback = [this, col] {
            CONSOLE_SCREEN_BUFFER_INFO info;
            GetConsoleScreenBufferInfo(this->state->output, &info);
            info.dwCursorPosition.X -= col;
            SetConsoleCursorPosition(this->state->output, info.dwCursorPosition);
        };
        if (this->disable_flush) {
            this->state->buffer.emplace_back(std::move(callback));
        } else {
            callback();
        }
    }

    void Win32Terminal::MoveForward(Column col) {
        auto callback = [this, col] {
            CONSOLE_SCREEN_BUFFER_INFO info;
            GetConsoleScreenBufferInfo(this->state->output, &info);
            info.dwCursorPosition.X += col;
            SetConsoleCursorPosition(this->state->output, info.dwCursorPosition);
        };
        if (this->disable_flush) {
            this->state->buffer.emplace_back(std::move(callback));
        } else {
            callback();
        }
    }

    void Win32Terminal::ShowCursor(bool show) {
        auto callback = [this, show] {
            CONSOLE_CURSOR_INFO info;
            GetConsoleCursorInfo(this->state->output, &info);
            info.bVisible = show;
            SetConsoleCursorInfo(this->state->output, &info);
        };
        if (this->disable_flush) {
            this->state->buffer.emplace_back(std::move(callback));
        } else {
            callback();
        }
    }

    void Win32Terminal::ClearScreen() {
        auto callback = [this] {
            CONSOLE_SCREEN_BUFFER_INFO info;
            GetConsoleScreenBufferInfo(this->state->output, &info);
            DWORD nbWritten;
            FillConsoleOutputCharacterA(this->state->output, ' ', info.dwSize.X * info.dwSize.Y, {0, 0}, &nbWritten);
            FillConsoleOutputAttribute(this->state->output, info.wAttributes, info.dwSize.X * info.dwSize.Y, {0, 0}, &nbWritten);
        };
        if (this->disable_flush) {
            this->state->buffer.emplace_back(std::move(callback));
        } else {
            callback();
        }
    }

    void Win32Terminal::ClearChars(Column col) {
        auto callback = [this, col] {
            CONSOLE_SCREEN_BUFFER_INFO info;
            GetConsoleScreenBufferInfo(this->state->output, &info);
            DWORD nbWritten;
            FillConsoleOutputCharacterA(this->state->output, ' ', col, info.dwCursorPosition, &nbWritten);
            FillConsoleOutputAttribute(this->state->output, info.wAttributes, col, info.dwCursorPosition, &nbWritten);
        };
        if (this->disable_flush) {
            this->state->buffer.emplace_back(std::move(callback));
        } else {
            callback();
        }
    }

    SlokedTerminal::Column Win32Terminal::GetWidth() {
        return this->state->terminalSize.column;
    }

    SlokedTerminal::Line Win32Terminal::GetHeight() {
        return this->state->terminalSize.line;
    }

    void Win32Terminal::Write(std::string_view text) {
        auto callback = [this, text = std::string{text}] {
            DWORD nbWritten, reserved;
            WriteConsoleA(this->state->output, text.data(), text.size(), &nbWritten, &reserved);
        };
        if (this->disable_flush) {
            this->state->buffer.emplace_back(std::move(callback));
        } else {
            callback();
        }
    }

    bool Win32Terminal::WaitInput(
        std::chrono::system_clock::duration timeout) {
        WaitForSingleObject(this->state->input, std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
        DWORD nbEvents{0};
        GetNumberOfConsoleInputEvents(this->state->input, &nbEvents);
        return nbEvents != 0;
    }

    static std::optional<SlokedKeyboardInput> KeyEventToSloked(const KEY_EVENT_RECORD &rec) {
        static const std::map<DWORD, SlokedControlKey> Keys {
            { VK_UP, SlokedControlKey::ArrowUp },
            { VK_DOWN, SlokedControlKey::ArrowDown },
            { VK_LEFT, SlokedControlKey::ArrowLeft },
            { VK_RIGHT, SlokedControlKey::ArrowRight },
            { VK_BACK, SlokedControlKey::Backspace },
            { VK_DELETE, SlokedControlKey::Delete },
            { VK_INSERT, SlokedControlKey::Insert },
            { VK_ESCAPE, SlokedControlKey::Escape },
            { VK_PRIOR, SlokedControlKey::PageUp },
            { VK_NEXT, SlokedControlKey::PageDown },
            { VK_HOME, SlokedControlKey::Home },
            { VK_END, SlokedControlKey::End },
            { VK_RETURN, SlokedControlKey::Enter },
            { VK_TAB, SlokedControlKey::Tab },
            { VK_F1, SlokedControlKey::F1 },
            { VK_F2, SlokedControlKey::F2 },
            { VK_F3, SlokedControlKey::F3 },
            { VK_F4, SlokedControlKey::F4 },
            { VK_F5, SlokedControlKey::F5 },
            { VK_F6, SlokedControlKey::F6 },
            { VK_F7, SlokedControlKey::F7 },
            { VK_F8, SlokedControlKey::F8 },
            { VK_F9, SlokedControlKey::F9 },
            { VK_F10, SlokedControlKey::F10 },
            { VK_F11, SlokedControlKey::F11 },
            { VK_F12, SlokedControlKey::F12 }
        };
        static const std::map<DWORD, SlokedControlKey> CtrlKeys {
            { VK_SPACE, SlokedControlKey::CtrlSpace },
            { 0x41, SlokedControlKey::CtrlA },
            { 0x42, SlokedControlKey::CtrlB },
            { 0x44, SlokedControlKey::CtrlD },
            { 0x45, SlokedControlKey::CtrlE },
            { 0x46, SlokedControlKey::CtrlF },
            { 0x47, SlokedControlKey::CtrlG },
            { 0x48, SlokedControlKey::CtrlH },
            { 0x4B, SlokedControlKey::CtrlK },
            { 0x4C, SlokedControlKey::CtrlL },
            { 0x4E, SlokedControlKey::CtrlN },
            { 0x4F, SlokedControlKey::CtrlO },
            { 0x50, SlokedControlKey::CtrlP },
            { 0x52, SlokedControlKey::CtrlR },
            { 0x54, SlokedControlKey::CtrlT },
            { 0x55, SlokedControlKey::CtrlU },
            { 0x56, SlokedControlKey::CtrlV },
            { 0x57, SlokedControlKey::CtrlW },
            { 0x58, SlokedControlKey::CtrlX },
            { 0x59, SlokedControlKey::CtrlY }
        };
        if (!rec.bKeyDown) {
            return {};
        }
        
        const bool AltPressed = (rec.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
        const bool CtrlPressed = (rec.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
        const bool AltGrPressed = (rec.dwControlKeyState & LEFT_CTRL_PRESSED) != 0 && (rec.dwControlKeyState & RIGHT_ALT_PRESSED) != 0;
        if (!AltGrPressed && !CtrlPressed && Keys.count(rec.wVirtualKeyCode) != 0) {
            return SlokedKeyboardInput {
                Keys.at(rec.wVirtualKeyCode), AltPressed
            };
        } else if (!AltGrPressed && CtrlPressed && CtrlKeys.count(rec.wVirtualKeyCode) != 0) {
            return SlokedKeyboardInput {
                CtrlKeys.at(rec.wVirtualKeyCode), AltPressed
            };
        }
        return SlokedKeyboardInput {
            Encoding.Encode(static_cast<char32_t>(rec.uChar.UnicodeChar)), AltPressed
        };
    }

    std::vector<SlokedKeyboardInput> Win32Terminal::GetInput() {
        DWORD maxNbEvents;
        GetNumberOfConsoleInputEvents(this->state->input, &maxNbEvents);
        std::unique_ptr<INPUT_RECORD[]> events{new INPUT_RECORD[maxNbEvents]};
        DWORD nbEvents{0};
        ReadConsoleInputW(this->state->input, events.get(), maxNbEvents, &nbEvents);
        std::vector<SlokedKeyboardInput> result;
        for (std::size_t i = 0; i < nbEvents; i++) {
            switch (events[i].EventType) {
                case KEY_EVENT: {
                    auto evt = KeyEventToSloked(events[i].Event.KeyEvent);
                    if (evt.has_value()) {
                        result.emplace_back(std::move(evt.value()));
                    }
                } break;

                case WINDOW_BUFFER_SIZE_EVENT: {
                    this->state->lastResize = std::chrono::system_clock::now();
                    SetConsoleTextAttribute(this->state->output, this->state->originalTextAttr);
                    DWORD nbWritten;
                    const DWORD count = events[i].Event.WindowBufferSizeEvent.dwSize.X * events[i].Event.WindowBufferSizeEvent.dwSize.Y;
                    FillConsoleOutputCharacterA(this->state->output, ' ', count, {0, 0}, &nbWritten);
                    FillConsoleOutputAttribute(this->state->output, this->state->originalTextAttr, count, {0, 0}, &nbWritten);
                    this->resizeEmitter.Emit();
                } break;
                
                default:
                    // Ignore event
                    break;
            }
        }
        return result;
    }

    void Win32Terminal::SetGraphicsMode(SlokedTextGraphics mode) {
        auto callback = [this, mode] {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(this->state->output, &csbi);
            switch (mode) {
                case SlokedTextGraphics::Off:
                    SetConsoleTextAttribute(this->state->output, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    break;

                case SlokedTextGraphics::Bold:
                    SetConsoleTextAttribute(this->state->output, csbi.wAttributes | FOREGROUND_INTENSITY);
                    break;

                case SlokedTextGraphics::Reverse:
                    SetConsoleTextAttribute(this->state->output, csbi.wAttributes | COMMON_LVB_REVERSE_VIDEO);
                    break;

                case SlokedTextGraphics::Underscore:
                    SetConsoleTextAttribute(this->state->output, csbi.wAttributes | COMMON_LVB_UNDERSCORE);
                    break;

                default:
                    // Not supported
                    break;
            }
        };
        if (this->disable_flush) {
            this->state->buffer.emplace_back(std::move(callback));
        } else {
            callback();
        }
    }

    void Win32Terminal::SetGraphicsMode(SlokedBackgroundGraphics mode) {
        auto callback = [this, mode] {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(this->state->output, &csbi);
            DWORD attr = csbi.wAttributes & ~(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
            switch (mode) {
                case SlokedBackgroundGraphics::Black:
                    break;

                case SlokedBackgroundGraphics::Blue:
                    attr |= BACKGROUND_BLUE;
                    break;

                case SlokedBackgroundGraphics::Cyan:
                    attr |= BACKGROUND_BLUE | BACKGROUND_GREEN;
                    break;

                case SlokedBackgroundGraphics::Green:
                    attr |= BACKGROUND_GREEN;
                    break;

                case SlokedBackgroundGraphics::Magenta:
                    attr |= BACKGROUND_RED | BACKGROUND_INTENSITY;
                    break;

                case SlokedBackgroundGraphics::Red:
                    attr |= BACKGROUND_RED;
                    break;

                case SlokedBackgroundGraphics::White:
                    attr |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
                    break;

                case SlokedBackgroundGraphics::Yellow:
                    attr |= BACKGROUND_RED | BACKGROUND_GREEN;
                    break;
            }
            SetConsoleTextAttribute(this->state->output, attr);
        };
        if (this->disable_flush) {
            this->state->buffer.emplace_back(std::move(callback));
        } else {
            callback();
        }
    }

    void Win32Terminal::SetGraphicsMode(SlokedForegroundGraphics mode) {
        auto callback = [this, mode] {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(this->state->output, &csbi);
            DWORD attr = csbi.wAttributes & ~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            switch (mode) {
                case SlokedForegroundGraphics::Black:
                    break;

                case SlokedForegroundGraphics::Blue:
                    attr |= FOREGROUND_BLUE;
                    break;

                case SlokedForegroundGraphics::Cyan:
                    attr |= FOREGROUND_BLUE | FOREGROUND_GREEN;
                    break;

                case SlokedForegroundGraphics::Green:
                    attr |= FOREGROUND_GREEN;
                    break;

                case SlokedForegroundGraphics::Magenta:
                    attr |= FOREGROUND_RED | FOREGROUND_INTENSITY;
                    break;

                case SlokedForegroundGraphics::Red:
                    attr |= FOREGROUND_RED;
                    break;

                case SlokedForegroundGraphics::White:
                    attr |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                    break;

                case SlokedForegroundGraphics::Yellow:
                    attr |= FOREGROUND_RED | FOREGROUND_GREEN;
                    break;
            }
            SetConsoleTextAttribute(this->state->output, attr);
        };
        if (this->disable_flush) {
            this->state->buffer.emplace_back(std::move(callback));
        } else {
            callback();
        }
    }

    bool Win32Terminal::UpdateDimensions() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(this->state->output, &csbi);
        TextPosition newSize {
            static_cast<TextPosition::Line>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1),
            static_cast<TextPosition::Column>(csbi.srWindow.Right - csbi.srWindow.Left + 1)
        };
        const bool updated = this->state->terminalSize != newSize;
        this->state->terminalSize = newSize;
        constexpr std::chrono::milliseconds LastUpdateTimeout{1000};
        return updated || this->state->lastResize + LastUpdateTimeout >=
               std::chrono::system_clock::now();
    }

    void Win32Terminal::Flush(bool flush) {
        this->disable_flush = flush;
        if (flush) {
            for (const auto &callback : this->state->buffer) {
                callback();
            }
            this->state->buffer.clear();
        }
    }
    
    std::function<void()> Win32Terminal::OnResize(std::function<void()> listener) {
        return this->resizeEmitter.Listen(std::move(listener));
    }
}