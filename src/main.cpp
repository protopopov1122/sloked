#include <cstdlib>
#include <iostream>
#include "sloked/text/TextDocument.h"
#include "sloked/text/TextView.h"
#include "sloked/text/TextChunk.h"
#include "sloked/text/cursor/TransactionCursor.h"
#include "sloked/text/cursor/TransactionBatch.h"
#include "sloked/text/cursor/TransactionStreamMultiplexer.h"
#include "sloked/screen/terminal/posix/PosixTerminal.h"
#include "sloked/screen/terminal/multiplexer/TerminalBuffer.h"
#include "sloked/screen/terminal/screen/ComponentHandle.h"
#include "sloked/screen/terminal/screen/SplitterComponent.h"
#include "sloked/screen/terminal/screen/TabberComponent.h"
#include "sloked/filesystem/posix/File.h"
#include "sloked/namespace/Filesystem.h"
#include "sloked/namespace/Virtual.h"
#include "sloked/namespace/View.h"
#include "sloked/namespace/posix/Filesystem.h"
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
}

int main(int argc, const char **argv) {
    if (argc < 3) {
        std::cout << "Format: " << argv[0] << " source destination" << std::endl;
        return EXIT_FAILURE;
    }

    char BUFFER[1024];
    realpath(argv[1], BUFFER);
    SlokedVirtualNamespace root(std::make_unique<SlokedFilesystemNamespace>(std::make_unique<SlokedPosixFilesystemAdapter>(SlokedPath::Root)));
    auto view = root.GetObject(BUFFER)->AsFile()->View();

    const Encoding &fileEncoding = Encoding::Utf8;
    const Encoding &terminalEncoding = Encoding::Utf8;
    EncodingConverter conv(fileEncoding, terminalEncoding);
    auto newline = NewLine::LF(fileEncoding);
    TextChunkFactory blockFactory(*newline);
    TextDocument text(*newline, TextView::Open(*view, *newline, blockFactory));

    TransactionStreamMultiplexer multiplexer(text, fileEncoding);
    auto stream1 = multiplexer.NewStream();
    auto stream2 = multiplexer.NewStream();
    TransactionCursor cursor1(text, fileEncoding, *stream1);
    TransactionCursor cursor2(text, fileEncoding, *stream2);
    TransactionCursor *cursor = &cursor1;
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
                of.close();
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
            cursor->Insert(conv.ReverseConvert(std::get<0>(cmd)));
        } else switch (std::get<1>(cmd)) {
            case SlokedControlKey::ArrowUp:
                cursor->MoveUp(1);
                break;
            
            case SlokedControlKey::ArrowDown:
                cursor->MoveDown(1);
                break;
            
            case SlokedControlKey::ArrowLeft:
                cursor->MoveBackward(1);
                break;
            
            case SlokedControlKey::ArrowRight:
                cursor->MoveForward(1);
                break;

            case SlokedControlKey::Enter:
                cursor->NewLine("");
                break;

            case SlokedControlKey::Tab:
                cursor->Insert(fileEncoding.Encode(u'\t'));
                break;

            case SlokedControlKey::Backspace:
                cursor->DeleteBackward();
                break;

            case SlokedControlKey::Delete:
                cursor->DeleteForward();
                break;

            case SlokedControlKey::F4:
                cursor->ClearRegion(TextPosition{cursor->GetLine() + 5, cursor->GetColumn() + 2});
                break;

            case SlokedControlKey::F5: {
                TransactionBatch batch(*stream2, fileEncoding);
                TransactionCursor crs(text, fileEncoding, batch);
                crs.ClearRegion(TextPosition{cursor->GetLine() + 10, cursor->GetColumn() + 2}, TextPosition{cursor->GetLine() + 15, cursor->GetColumn() + 2});
                crs.ClearRegion(TextPosition{cursor->GetLine(), cursor->GetColumn() + 2}, TextPosition{cursor->GetLine() + 5, cursor->GetColumn() + 2});
                batch.Finish();
            } break;
            
            case SlokedControlKey::Escape:
                cursor->Undo();
                break;
            
            case SlokedControlKey::End:
                cursor->Redo();
                break;

            case SlokedControlKey::PageUp:
                cursor = &cursor1;
                break;

            case SlokedControlKey::PageDown:
                cursor = &cursor2;
                break;

            default:
                break;
        }
        if (cursor->GetLine() - offset >= buffer && cursor->GetLine() >= buffer) {
            offset = cursor->GetLine() - buffer;
        }
        while (cursor->GetLine() + 1 <= offset && offset > 0) {
            offset--;
        }
        return true;
    };
    screen.SetInputHandler(listener1);

    tab1.SetInputHandler(listener2);
    tab1.SetRenderer([&](auto &term) {
        term.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
        print(text, term, conv);
        term.SetPosition(cursor->GetLine() - offset, charWidth.GetRealPosition(
            std::string {text.GetLine(cursor->GetLine())},
            cursor->GetColumn(),
            fileEncoding
        ));
    });

    tab2.SetInputHandler(listener2);
    tab2.SetRenderer([&](auto &term) {
        term.SetGraphicsMode(SlokedTextGraphics::Off);
        term.SetGraphicsMode(SlokedBackgroundGraphics::Yellow);
        print(text, term, conv);
        term.SetPosition(cursor->GetLine() - offset, charWidth.GetRealPosition(
            std::string {text.GetLine(cursor->GetLine())},
            cursor->GetColumn(),
            fileEncoding
        ));
    });

    tab3.SetInputHandler(listener2);
    tab3.SetRenderer([&](auto &term) {
        term.SetGraphicsMode(SlokedTextGraphics::Off);
        term.SetGraphicsMode(SlokedBackgroundGraphics::Red);
        print(text, term, conv);
        term.SetPosition(cursor->GetLine() - offset, charWidth.GetRealPosition(
            std::string {text.GetLine(cursor->GetLine())},
            cursor->GetColumn(),
            fileEncoding
        ));
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
