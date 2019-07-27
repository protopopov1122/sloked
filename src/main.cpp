#include <cstdlib>
#include <iostream>
#include "sloked/screen/posix-term/PosixTerminal.h"
#include "sloked/screen/term-multiplexer/TerminalWindow.h"
#include "sloked/screen/term-multiplexer/TerminalBuffer.h"
#include "sloked/screen/term-multiplexer/TerminalSplitter.h"
#include "sloked/screen/term-multiplexer/TerminalTabber.h"
#include "sloked/text/TextChunk.h"
#include "sloked/text/TextBlockHandle.h"
#include "sloked/text/TextDocument.h"
#include "sloked/text/TextView.h"
#include "sloked/text/posix/TextFile.h"
#include "sloked/text/Cursor.h"
#include "sloked/screen/CharWidth.h"
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
    SlokedCursor cursor(text, Encoding::Utf8);
    ScreenCharWidth charWidth;

    PosixTerminal terminal;
    BufferedTerminal console(terminal, Encoding::Utf8, charWidth);
    TerminalSplitter splitter(console, TerminalSplitter::Direction::Horizontal, Encoding::Utf8, charWidth);
    TerminalTabber tabber(splitter.NewWindow(TerminalSplitter::Constraints(0.6)));
    auto &tab1 = tabber.NewTab();
    auto &tab2 = tabber.NewTab();
    auto &tab3 = tabber.NewTab();
    auto &pane4 = splitter.NewWindow(TerminalSplitter::Constraints(0.3));

    const auto flush = [&]() {
        console.SetGraphicsMode(SlokedTerminalText::Off);
        console.ClearScreen();
        tab1.SetGraphicsMode(SlokedTerminalBackground::Blue);
        print(text, tab1);
        tab2.SetGraphicsMode(SlokedTerminalText::Off);
        tab2.SetGraphicsMode(SlokedTerminalBackground::Yellow);
        print(text, tab2);
        tab3.SetGraphicsMode(SlokedTerminalText::Off);
        tab3.SetGraphicsMode(SlokedTerminalBackground::Red);
        print(text, tab3);
        pane4.SetGraphicsMode(SlokedTerminalBackground::Magenta);
        print(text, pane4);
        tabber.GetTab(tabber.GetCurrentTab())->SetPosition(cursor.GetLine() - offset, charWidth.GetRealPosition(std::string {text.GetLine(cursor.GetLine())}, cursor.GetColumn(), Encoding::Utf8));
        console.Flush();
    };
    
    buffer = terminal.GetHeight() - 1;
    flush();
    while (true) {
        auto input = tabber.GetTab(tabber.GetCurrentTab())->GetInput();

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
                
                case SlokedControlKey::F9: {
                    std::ofstream of(argv[2]);
                    of << text;
                    return EXIT_SUCCESS;
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
                    break;
            }
            if (cursor.GetLine() - offset >= buffer && cursor.GetLine() >= buffer) {
                offset = cursor.GetLine() - buffer;
            }
            if (cursor.GetLine() <= offset) {
                offset -= offset - cursor.GetLine();
            }
        }

        flush();
    }
    return EXIT_SUCCESS;
}
