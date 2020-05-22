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
        }

        ~State() {
            CONSOLE_SCREEN_BUFFER_INFO info;
            GetConsoleScreenBufferInfo(this->output, &info);
            DWORD nbWritten;
            FillConsoleOutputCharacterA(this->output, ' ', info.dwSize.X * info.dwSize.Y, {0, 0}, &nbWritten);
            SetConsoleMode(this->input, this->originalMode);
            SetConsoleCP(this->oldCodepage);
            SetConsoleOutputCP(this->oldOutputCodepage);
        }

        HANDLE output;
        HANDLE input;
        DWORD originalMode;
        UINT oldCodepage;
        UINT oldOutputCodepage;
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
        COORD pos;
        pos.X = column;
        pos.Y = line;
        SetConsoleCursorPosition(this->state->output, pos);
    }

    void Win32Terminal::MoveUp(Line line) {
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(this->state->output, &info);
        info.dwCursorPosition.Y -= line;
        SetConsoleCursorPosition(this->state->output, info.dwCursorPosition);
    }

    void Win32Terminal::MoveDown(Line line) {
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(this->state->output, &info);
        info.dwCursorPosition.Y += line;
        SetConsoleCursorPosition(this->state->output, info.dwCursorPosition);
    }

    void Win32Terminal::MoveBackward(Column col) {
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(this->state->output, &info);
        info.dwCursorPosition.X -= col;
        SetConsoleCursorPosition(this->state->output, info.dwCursorPosition);
    }

    void Win32Terminal::MoveForward(Column col) {
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(this->state->output, &info);
        info.dwCursorPosition.X += col;
        SetConsoleCursorPosition(this->state->output, info.dwCursorPosition);
    }

    void Win32Terminal::ShowCursor(bool show) {
        CONSOLE_CURSOR_INFO info;
        GetConsoleCursorInfo(this->state->output, &info);
        info.bVisible = show;
        SetConsoleCursorInfo(this->state->output, &info);
    }

    void Win32Terminal::ClearScreen() {
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(this->state->output, &info);
        DWORD nbWritten;
        FillConsoleOutputCharacterA(this->state->output, ' ', info.dwSize.X * info.dwSize.Y, {0, 0}, &nbWritten);
    }

    void Win32Terminal::ClearChars(Column col) {
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(this->state->output, &info);
        DWORD nbWritten;
        FillConsoleOutputCharacterA(this->state->output, ' ', col, info.dwCursorPosition, &nbWritten);
    }

    SlokedTerminal::Column Win32Terminal::GetWidth() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(this->state->output, &csbi);
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }

    SlokedTerminal::Line Win32Terminal::GetHeight() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(this->state->output, &csbi);
        return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }

    void Win32Terminal::Write(std::string_view text) {
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(this->state->output, &info);
        DWORD nbWritten;
        WriteConsoleOutputCharacterA(this->state->output, text.data(), text.size(), info.dwCursorPosition, &nbWritten);
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
                
                default:
                    // Ignore event
                    break;
            }
        }
        return result;
    }

    void Win32Terminal::SetGraphicsMode(SlokedTextGraphics) {}
    void Win32Terminal::SetGraphicsMode(SlokedBackgroundGraphics) {}
    void Win32Terminal::SetGraphicsMode(SlokedForegroundGraphics) {}

    bool Win32Terminal::UpdateDimensions() {
        return false;
    }

    void Win32Terminal::Flush(bool flush) {
        this->disable_flush = flush;
    }
}