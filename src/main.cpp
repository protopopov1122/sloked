#include <cstdlib>
#include <iostream>
#include "sloked/screen/posix-term/PosixTerminal.h"
#include "sloked/screen/term-multiplexer/TerminalWindow.h"
#include "sloked/screen/term-multiplexer/TerminalBuffer.h"
#include "sloked/screen/term-multiplexer/TerminalScreenView.h"
#include "sloked/screen/term-multiplexer/TerminalSplitter.h"
#include "sloked/text/TextChunk.h"
#include "sloked/text/TextBlockHandle.h"
#include "sloked/text/TextDocument.h"
#include "sloked/text/TextView.h"
#include "sloked/text/posix/TextFile.h"
#include "sloked/text/Cursor.h"
#include <fcntl.h>
#include <fstream>
#include <sstream>

using namespace sloked;

static std::size_t offset = 0;
static std::size_t buffer = 24;

void print(TextBlock &text, SlokedTerminal &console) {
    console.ClearScreen();
    console.SetPosition(0, 0);
    std::stringstream ss;
    text.Visit(offset, std::min(buffer, text.GetLastLine() - offset) + 1, [&console, &ss](const auto line) {
        ss << line << std::endl;
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
    TextChunkFactory blockFactory(NewLine::AnsiLF);
    TextDocument text(NewLine::AnsiLF, TextView::Open(view, NewLine::AnsiLF, blockFactory));

    PosixTerminal terminal;
    BufferedTerminal buffConsole(terminal, Encoding::Utf8);
    TerminalSplitter splitter(buffConsole, TerminalSplitter::Direction::Horizontal, Encoding::Utf8);
    auto &console1 = splitter.NewWindow(TerminalSplitter::Constraints(0.3));
    auto &console2 = splitter.NewWindow(TerminalSplitter::Constraints(0.4, 0, 15));
    auto &temp = splitter.NewWindow(TerminalSplitter::Constraints(0.3));
    TerminalSplitter splitter2(temp, TerminalSplitter::Direction::Vertical, Encoding::Utf8);
    auto &console3 = splitter2.NewWindow(TerminalSplitter::Constraints(0.5));
    print(text, console1);

    console3.SetGraphicsMode(SlokedTerminalText::Off);
    console3.SetGraphicsMode(SlokedTerminalBackground::Red);
    console3.ClearScreen();

    buffConsole.Flush();
    
    buffer = console1.GetHeight() - 3;
    console1.SetPosition(0, 0);
    SlokedCursor cursor(text, Encoding::Utf8);
    while (true) {
        auto input = console1.GetInput();

        for (const auto &cmd : input) {
            if (cmd.index() == 0) {
                cursor.Insert(std::get<0>(cmd));
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

                case SlokedControlKey::Backspace:
                    cursor.Remove();
                    break;
                
                case SlokedControlKey::F1: {
                    std::ofstream of(argv[2]);
                    of << text;
                    return EXIT_SUCCESS;
                }

                default:
                    break;
            }
            if (cursor.GetLine() - offset >= buffer && cursor.GetLine() >= buffer) {
                offset = cursor.GetLine() - buffer;
            }
            if (cursor.GetLine() <= offset) {
                offset -= offset - cursor.GetLine();
            }
        }

        console1.SetGraphicsMode(SlokedTerminalBackground::Blue);
        print(text, console1);
        console2.SetGraphicsMode(SlokedTerminalText::Off);
        console2.SetGraphicsMode(SlokedTerminalBackground::Yellow);
        print(text, console2);
        console2.SetGraphicsMode(SlokedTerminalText::Off);
        console2.SetGraphicsMode(SlokedTerminalBackground::Red);
        print(text, console3);
        console1.SetPosition(cursor.GetLine() - offset, cursor.GetColumn());
        buffConsole.Flush();
    }
    return EXIT_SUCCESS;
}
