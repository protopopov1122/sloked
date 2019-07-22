#include <cstdlib>
#include <iostream>
#include "sloked/PosixAnsiConsole.h"
#include "sloked/text/TextChunk.h"
#include "sloked/text/TextBlockHandle.h"
#include "sloked/text/TextDocument.h"
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

void print(TextBlock &text, PosixAnsiConsole &console) {
    console.ClearScreen();
    console.SetPosition(1, 1);
    std::stringstream ss;
    text.Visit(offset, std::min(buffer, text.GetLastLine() - offset) + 1, [&console, &ss](const auto line) {
        ss << line << std::endl;
    });
    console.Write(ss.str());
    console.SetPosition(1, 1);
}

int main(int argc, const char **argv) {
    if (argc < 3) {
        std::cout << "Format: " << argv[0] << " source destination" << std::endl;
        return EXIT_FAILURE;
    }
    PosixAnsiConsole console; 
    TextDocument text(NewLine::LF, std::make_unique<PosixTextFile>(NewLine::LF, open(argv[1], O_RDONLY, 0)));

    int chr;
    print(text, console);
    int line = 0;
    int col = 0;
    while ((chr = console.GetChar()) != EOF) {
        if (chr == '\033') {
            int next = console.GetChar();
            if (next == '[') {
                int cmd = console.GetChar();
                console.ClearScreen();
                console.Write(std::to_string(cmd));
                switch (cmd) {
                    case 'A':
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
                    case 'B':
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
                    case 'C':
                        col++;
                        if (col > text.GetLine(offset + line).size()) {
                            col = text.GetLine(offset + line).size();
                        }
                        if (col < 0) {
                            col = 0;
                        }
                        break;
                    case 'D':
                        col--;
                        if (col < 0) {
                            col = 0;
                        }
                        break;
                }
            } else if (next == 'O') {
                int cmd = console.GetChar();
                switch (cmd) {
                    case 'P':
                        std::ofstream of(argv[2]);
                        of << text;
                        return EXIT_SUCCESS;
                }
            }
        } else if (chr == 127) {
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
        } else if (chr == '\n') {
            std::string ln{text.GetLine(offset + line)};
            text.SetLine(offset + line, ln.substr(0, col));
            text.InsertLine(offset + line, ln.substr(col));
            line++;
            col = 0;
            if (line > buffer) {
                offset++;
                line--;
            }
        } else {
            std::string ln{text.GetLine(offset + line)};
            char tmp[] = {chr, '\0'};
            text.SetLine(offset + line, ln.substr(0, col) + std::string(tmp) + ln.substr(col));
            col++;
        }
        print(text, console);
        console.SetPosition(line + 1, col + 1);
    }
    return EXIT_SUCCESS;
}
