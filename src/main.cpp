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
#include <unistd.h>
#include <cstdio>
#include <sys/types.h>
#include <fcntl.h>
#include <fstream>
#include <cstring>
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
    TextChunkFactory blockFactory(NewLine::LF);
    TextDocument text(NewLine::LF, TextView::Open(view, NewLine::LF, blockFactory));

    PosixTerminal terminal;
    BufferedTerminal buffConsole(terminal);
    TerminalSplitter splitter(buffConsole, TerminalSplitter::Direction::Horizontal);
    auto &console1 = splitter.NewWindow(TerminalSplitter::Constraints(0.3));
    auto &console2 = splitter.NewWindow(TerminalSplitter::Constraints(0.4, 0, 15));
    auto &temp = splitter.NewWindow(TerminalSplitter::Constraints(0.3));
    TerminalSplitter splitter2(temp, TerminalSplitter::Direction::Vertical);
    auto &console3 = splitter2.NewWindow(TerminalSplitter::Constraints(0.5));
    print(text, console1);

    buffConsole.Flush();
    buffer = console1.GetHeight() - 3;
    console1.SetPosition(0, 0);
    int line = 0;
    int col = 0;
    while (true) {
        auto input = console1.GetInput();

        for (const auto &cmd : input) {
            if (cmd.index() == 0) {
                std::string ln{text.GetLine(offset + line)};
                text.SetLine(offset + line, ln.substr(0, col) + std::get<0>(cmd) + ln.substr(col));
                col += std::get<0>(cmd).size();
            } else switch (std::get<1>(cmd)) {
                case SlokedControlKey::ArrowUp:
                    line--;
                    if (line < 0) {
                        line = 0;
                        offset--;
                        if (offset > text.GetLastLine()) {
                            offset = 0;
                        }
                    }
                    if (col > text.GetLine(offset + line).size()) {
                        col = text.GetLine(offset + line).size();
                    }
                    if (col < 0) {
                        col = 0;
                    }
                    break;
                
                case SlokedControlKey::ArrowDown:
                    line++;
                    if (line > buffer) {
                        line = buffer;
                        offset++;
                    }
                    if (offset + line > text.GetLastLine()) {
                        offset = text.GetLastLine() - line;
                    }
                    if (col > text.GetLine(offset + line).size()) {
                        col = text.GetLine(offset + line).size();
                    }
                    if (col < 0) {
                        col = 0;
                    }
                    break;
                
                case SlokedControlKey::ArrowLeft:
                    col--;
                    if (col < 0) {
                        col = 0;
                    }
                    break;
                
                case SlokedControlKey::ArrowRight:
                    col++;
                    if (col > text.GetLine(offset + line).size()) {
                        col = text.GetLine(offset + line).size();
                    }
                    if (col < 0) {
                        col = 0;
                    }
                    break;

                case SlokedControlKey::Enter: {
                    std::string ln{text.GetLine(offset + line)};
                    text.SetLine(offset + line, ln.substr(0, col));
                    text.InsertLine(offset + line, ln.substr(col));
                    line++;
                    col = 0;
                    if (line > buffer) {
                        offset++;
                        line--;
                    }
                } break;

                case SlokedControlKey::Backspace:
                    if (col > 0) {
                        std::string ln{text.GetLine(offset + line)};
                        text.SetLine(offset + line, ln.substr(0, col - 1) + ln.substr(col));
                        if (col > 0) {
                            col--;
                        }
                    } else if (offset + line > 0) {
                        std::string ln1{text.GetLine(offset + line - 1)};
                        std::string ln2{text.GetLine(offset + line)};
                        text.SetLine(offset + line - 1, ln1 + ln2);
                        text.EraseLine(offset + line);
                        if (line == 0) {
                            offset--;
                        } else {
                            line--;
                        }
                        col = ln1.size();
                    }
                    break;
                
                case SlokedControlKey::F1: {
                    std::ofstream of(argv[2]);
                    of << text;
                    return EXIT_SUCCESS;
                }

                default:
                    break;
            }
        }

        console1.SetGraphicsMode(SlokedTerminalBackground::Blue);
        print(text, console1);
        console2.SetGraphicsMode(SlokedTerminalText::Off);
        console2.SetGraphicsMode(SlokedTerminalBackground::Yellow);
        print(text, console2);
        console3.SetGraphicsMode(SlokedTerminalText::Off);
        console3.SetGraphicsMode(SlokedTerminalBackground::Red);
        print(text, console3);
        console1.SetPosition(line, col);
        buffConsole.Flush();
    }
    return EXIT_SUCCESS;
}
