#include <cstdlib>
#include <iostream>
#include "sloked/text/TextDocument.h"
#include "sloked/text/TextView.h"
#include "sloked/text/posix/TextFile.h"
#include "sloked/text/Cursor.h"
#include "sloked/screen/terminal/posix/PosixTerminal.h"
#include "sloked/screen/terminal/multiplexer/TerminalBuffer.h"
#include "sloked/screen/terminal/screen/ComponentHandle.h"
#include "sloked/screen/terminal/screen/SplitterComponent.h"
#include "sloked/screen/terminal/screen/TabberComponent.h"
#include <fcntl.h>
#include <fstream>
#include <sstream>

using namespace sloked;

static std::size_t offset = 0;
static std::size_t buffer = 24;

void print(TextBlock &text, SlokedTextPane &console, const EncodingConverter &conv) {
    console.ClearScreen();
    console.SetPosition(0, 0);
    std::stringstream ss;
    text.Visit(offset, std::min(buffer, text.GetLastLine() - offset) + 1, [&](const auto line) {
        ss << conv.Convert(line) << std::endl;
    });
    console.Write(ss.str());
    console.SetPosition(0, 0);
}

int main(int argc, const char **argv) {
    if (argc < 3) {
        std::cout << "Format: " << argv[0] << " source destination" << std::endl;
        return EXIT_FAILURE;
    }
    PosixTextView view(open(argv[1], O_RDONLY, 0));
    const Encoding &fileEncoding = Encoding::Utf8;
    const Encoding &terminalEncoding = Encoding::Utf8;
    EncodingConverter conv(fileEncoding, terminalEncoding);
    auto newline = NewLine::LF(fileEncoding);
    TextChunkFactory blockFactory(*newline);
    TextDocument text(*newline, TextView::Open(view, *newline, blockFactory));
    SlokedCursor cursor(text, fileEncoding);
    ScreenCharWidth charWidth;

    PosixTerminal terminal;
    BufferedTerminal console(terminal, Encoding::Utf8, charWidth);
    buffer = console.GetHeight() - 3;

    TerminalComponentHandle screen(console, terminalEncoding, charWidth);
    auto &splitter = screen.NewSplitter(Splitter::Direction::Horizontal);
    auto &tabber = splitter.NewWindow(Splitter::Constraints(0.6)).NewTabber();
    auto &tab1 = tabber.NewTab().NewTextPane();
    auto &tab2 = tabber.NewTab().NewTextPane();
    auto &tab3 = tabber.NewTab().NewTextPane();
    auto &pane4 = splitter.NewWindow(Splitter::Constraints(0.3)).NewTextPane();

    auto listener1 = [&](const SlokedKeyboardInput &cmd) {
        if (cmd.index() == 0) {
            return false;
        } else switch (std::get<1>(cmd)) {            
            case SlokedControlKey::F9: {
                std::ofstream of(argv[2]);
                of << text;
                std::exit(EXIT_SUCCESS);
            }

            case SlokedControlKey::F1:
                tabber.SelectTab(0);
                break;

            case SlokedControlKey::F2:
                tabber.SelectTab(1);
                break;

            case SlokedControlKey::F3:
                tabber.SelectTab(2);
                break;

            default:
                return false;
        }
        return true;
    };

    auto listener2 = [&](const SlokedKeyboardInput &cmd) {
        if (cmd.index() == 0) {
            cursor.Insert(conv.ReverseConvert(std::get<0>(cmd)));
        } else switch (std::get<1>(cmd)) {
            case SlokedControlKey::ArrowUp:
                cursor.MoveUp(1);
                break;
            
            case SlokedControlKey::ArrowDown:
                cursor.MoveDown(1);
                break;
            
            case SlokedControlKey::ArrowLeft:
                cursor.MoveBackward(1);
                break;
            
            case SlokedControlKey::ArrowRight:
                cursor.MoveForward(1);
                break;

            case SlokedControlKey::Enter:
                cursor.NewLine();
                break;

            case SlokedControlKey::Tab:
                cursor.Insert(fileEncoding.Encode(u'\t'));
                break;

            case SlokedControlKey::Backspace:
                cursor.Remove();
                break;

            default:
                break;
        }
        if (cursor.GetLine() - offset >= buffer && cursor.GetLine() >= buffer) {
            offset = cursor.GetLine() - buffer;
        }
        while (cursor.GetLine() + 1 <= offset && offset > 0) {
            offset--;
        }
        return true;
    };
    screen.SetInputHandler(listener1);

    tab1.SetInputHandler(listener2);
    tab1.SetRenderer([&](auto &term) {
        term.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
        print(text, term, conv);
        term.SetPosition(cursor.GetLine() - offset, cursor.GetColumn());
    });

    tab2.SetInputHandler(listener2);
    tab2.SetRenderer([&](auto &term) {
        term.SetGraphicsMode(SlokedTextGraphics::Off);
        term.SetGraphicsMode(SlokedBackgroundGraphics::Yellow);
        print(text, term, conv);
        term.SetPosition(cursor.GetLine() - offset, cursor.GetColumn());
    });

    tab3.SetInputHandler(listener2);
    tab3.SetRenderer([&](auto &term) {
        term.SetGraphicsMode(SlokedTextGraphics::Off);
        term.SetGraphicsMode(SlokedBackgroundGraphics::Red);
        print(text, term, conv);
        term.SetPosition(cursor.GetLine() - offset, cursor.GetColumn());
    });

    pane4.SetRenderer([&](auto &term) {
        term.SetGraphicsMode(SlokedTextGraphics::Off);
        term.SetGraphicsMode(SlokedBackgroundGraphics::Magenta);
        print(text, term, conv);
    });

    do {
        console.Update();
        console.SetGraphicsMode(SlokedTextGraphics::Off);
        console.ClearScreen();
        screen.Render();
        console.Flush();
        auto input = console.GetInput();
        for (const auto &evt : input) {
            screen.ProcessInput(evt);
        }
    } while (true);

    return EXIT_SUCCESS;
}
