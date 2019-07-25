#include <cstdlib>
#include <iostream>
#include "sloked/screen/posix-term/PosixTerminal.h"
#include "sloked/screen/term-multiplexer/TerminalWindow.h"
#include "sloked/screen/term-multiplexer/TerminalBuffer.h"
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
    BufferedTerminal console(terminal);

    TerminalWindow console1(console, 10, 10, 90, 26, [&]() { return console.GetInput(); });
    TerminalWindow console2(console, 20, 20, 90, 26, [&]() { return std::vector<SlokedKeyboardInput>{}; });
    TerminalWindow console3(console, 95, 15, 90, 26, [&]() { return std::vector<SlokedKeyboardInput>{}; });

    console.ClearScreen();
    console1.SetGraphicsMode(SlokedTerminalBackground::Blue);
    console1.ClearScreen();
    print(text, console1);
    console2.SetGraphicsMode(SlokedTerminalText::Off);
    console2.SetGraphicsMode(SlokedTerminalText::Bold);
    console2.ClearScreen();
    print(text, console2);
    console3.SetGraphicsMode(SlokedTerminalText::Off);
    console3.SetGraphicsMode(SlokedTerminalBackground::Red);
    console3.ClearScreen();
    print(text, console3);
    
    console.Flush();
    buffer = console1.GetHeight() - 2;
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
        console2.SetGraphicsMode(SlokedTerminalText::Bold);
        print(text, console2);
        console3.SetGraphicsMode(SlokedTerminalText::Off);
        console3.SetGraphicsMode(SlokedTerminalBackground::Red);
        print(text, console3);
        console1.SetPosition(line, col);
        console.Flush();
    }
    return EXIT_SUCCESS;
}
