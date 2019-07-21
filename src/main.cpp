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

using namespace sloked;

static std::size_t offset = 0;
static std::size_t buffer = 24;

void print(TextBlock &chunk, PosixAnsiConsole &console) {
    console.ClearScreen();
    console.SetPosition(1, 1);
    for (std::size_t i = offset; i <= std::min(offset + buffer, chunk.GetLastLine()); i++) {
        console.Write(std::string(chunk.GetLine(i)) + "\n");
    }
    console.SetPosition(1, 1);
}

int main(int argc, const char **argv) {
    PosixAnsiConsole console; 
    TextDocument chunk(std::make_unique<PosixTextFile>(open("hamlet.txt", O_RDONLY, 0)));

    int chr;
    print(chunk, console);
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
                            if (offset > chunk.GetLastLine()) {
                                offset = 0;
                            }
                        }
                        if (col > chunk.GetLine(offset + line).size()) {
                            col = chunk.GetLine(offset + line).size();
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
                            if (offset + line > chunk.GetLastLine()) {
                                offset = chunk.GetLastLine() - line;
                            }
                        }
                        if (col > chunk.GetLine(offset + line).size()) {
                            col = chunk.GetLine(offset + line).size();
                        }
                        if (col < 0) {
                            col = 0;
                        }
                        break;
                    case 'C':
                        col++;
                        if (col > chunk.GetLine(offset + line).size()) {
                            col = chunk.GetLine(offset + line).size();
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
                        std::ofstream of("res.txt");
                        of << chunk;
                        return EXIT_SUCCESS;
                }
            }
        } else if (chr == 127) {
            if (col > 0) {
                std::string ln{chunk.GetLine(offset + line)};
                chunk.SetLine(offset + line, ln.substr(0, col - 1) + ln.substr(col));
                if (col > 0) {
                    col--;
                }
            } else if (offset + line > 0) {
                std::string ln1{chunk.GetLine(offset + line - 1)};
                std::string ln2{chunk.GetLine(offset + line)};
                chunk.SetLine(offset + line - 1, ln1 + ln2);
                chunk.EraseLine(offset + line);
                if (line == 0) {
                    offset--;
                } else {
                    line--;
                }
                col = ln1.size();
            }
        } else if (chr == '\n') {
            std::string ln{chunk.GetLine(offset + line)};
            chunk.SetLine(offset + line, ln.substr(0, col));
            chunk.InsertLine(offset + line, ln.substr(col));
            line++;
            col = 0;
            if (line > buffer) {
                offset++;
                line--;
            }
        } else {
            std::string ln{chunk.GetLine(offset + line)};
            char tmp[] = {chr, '\0'};
            std::cout << col << std::endl;
            chunk.SetLine(offset + line, ln.substr(0, col) + std::string(tmp) + ln.substr(col));
            col++;
        }
        chunk.Optimize();
        print(chunk, console);
        console.SetPosition(line + 1, col + 1);
    }
    return EXIT_SUCCESS;
}