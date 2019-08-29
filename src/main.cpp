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
#include "sloked/screen/terminal/components/ComponentHandle.h"
#include "sloked/screen/terminal/components/SplitterComponent.h"
#include "sloked/screen/terminal/components/TabberComponent.h"
#include "sloked/screen/terminal/components/MultiplexerComponent.h"
#include "sloked/screen/widgets/TextEditor.h"
#include "sloked/filesystem/posix/File.h"
#include "sloked/namespace/Filesystem.h"
#include "sloked/namespace/Virtual.h"
#include "sloked/namespace/View.h"
#include "sloked/namespace/posix/Filesystem.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/text/fragment/Updater.h"
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <queue>

using namespace sloked;

class TestFragment : public SlokedTextTagger<int> {
 public:
    TestFragment(const TextBlockView &text, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : text(text), encoding(encoding), charWidth(charWidth), current{0, 0} {}

    std::optional<TaggedTextFragment<int>> Next() override {
        if (this->cache.empty()) {
            this->ParseLine();
        }
        if (!this->cache.empty()) {
            auto fragment = std::move(this->cache.front());
            this->cache.pop();
            return fragment;
        } else {
            return {};
        }
    }
    
    void Rewind(const TextPosition &position) override {
        if (position < this->current) {
            this->current = position;
            this->cache = {};
        }
    }

 private:
    void ParseLine() {
        if (this->current.line > this->text.GetLastLine()) {
            return;
        }

        const auto TabLength = this->encoding.CodepointCount("\t");
        std::string currentLine{this->text.GetLine(this->current.line)};
        auto lineLength = this->encoding.CodepointCount(currentLine);
        while (this->current.column < lineLength) {
            auto position = this->encoding.GetCodepoint(currentLine, this->current.column);
            auto substr = currentLine.substr(position.first, 1);
            if (substr.compare("\t") == 0) {
                this->cache.push(TaggedTextFragment<int>(this->current, TextPosition{0, static_cast<TextPosition::Column>(TabLength)}, 1));
            }
            this->current.column++;
        }

        this->current.line++;
        this->current.column = 0;
    }

    const TextBlockView &text;
    const Encoding &encoding;
    const SlokedCharWidth &charWidth;
    TextPosition current;
    std::queue<TaggedTextFragment<int>> cache;
};

int main(int argc, const char **argv) {
    if (argc < 3) {
        std::cout << "Format: " << argv[0] << " source destination" << std::endl;
        return EXIT_FAILURE;
    }

    char BUFFER[1024];
    realpath(argv[1], BUFFER);
    SlokedVirtualNamespace root(std::make_unique<SlokedFilesystemNamespace>(std::make_unique<SlokedPosixFilesystemAdapter>("/")));
    auto view = root.GetObject(BUFFER)->AsFile()->View();

    const Encoding &fileEncoding = Encoding::Utf8;
    const Encoding &terminalEncoding = Encoding::Utf8;
    EncodingConverter conv(fileEncoding, terminalEncoding);
    auto newline = NewLine::LF(fileEncoding);
    TextChunkFactory blockFactory(*newline);
    TextDocument text(*newline, TextView::Open(*view, *newline, blockFactory));

    TransactionStreamMultiplexer multiplexer(text, fileEncoding);
    auto stream1 = multiplexer.NewStream();
    TransactionCursor cursor(text, fileEncoding, *stream1);
    SlokedCharWidth charWidth;

    SlokedLazyTaggedText<int> lazyTags(std::make_unique<TestFragment>(text, fileEncoding, charWidth));
    SlokedCacheTaggedText<int> tags(lazyTags);
    auto fragmentUpdater = std::make_shared<SlokedFragmentUpdater<int>>(text, tags, fileEncoding, charWidth);
    stream1->AddListener(fragmentUpdater);

    PosixTerminal terminal;
    BufferedTerminal console(terminal, Encoding::Utf8, charWidth);

    TerminalComponentHandle screen(console, terminalEncoding, charWidth);
    auto &multi = screen.NewMultiplexer();
    auto win1 = multi.NewWindow(TextPosition{5, 5}, TextPosition{40, 150});
    auto win2 = multi.NewWindow(TextPosition{10, 50}, TextPosition{30, 150});
    auto &splitter = win1->GetComponent().NewSplitter(Splitter::Direction::Horizontal);
    auto &tabber = splitter.NewWindow(Splitter::Constraints(0.4))->GetComponent().NewTabber();
    auto &tab1 = tabber.NewWindow()->GetComponent().NewTextPane(std::make_unique<SlokedTextEditor>(text, cursor, cursor, tags, conv, charWidth));
    auto &tab2 = tabber.NewWindow()->GetComponent().NewTextPane(std::make_unique<SlokedTextEditor>(text, cursor, cursor, tags, conv, charWidth, SlokedBackgroundGraphics::Blue));
    auto &tab3 = tabber.NewWindow()->GetComponent().NewTextPane(std::make_unique<SlokedTextEditor>(text, cursor, cursor, tags, conv, charWidth, SlokedBackgroundGraphics::Magenta));
    auto &pane4 = splitter.NewWindow(Splitter::Constraints(0.2))->GetComponent().NewTextPane(std::make_unique<SlokedTextEditor>(text, cursor, cursor, tags, conv, charWidth));
    auto &pane5 = splitter.NewWindow(Splitter::Constraints(0.15))->GetComponent().NewTextPane(std::make_unique<SlokedTextEditor>(text, cursor, cursor, tags, conv, charWidth));
    auto &pane6 = win2->GetComponent().NewTextPane(std::make_unique<SlokedTextEditor>(text, cursor, cursor, tags, conv, charWidth, SlokedBackgroundGraphics::Yellow));
    win1->SetFocus();

    screen.SetInputHandler([&](const SlokedKeyboardInput &cmd) {
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
                tabber.GetWindow(0)->SetFocus();
                break;

            case SlokedControlKey::F2:
                tabber.GetWindow(1)->SetFocus();
                break;

            case SlokedControlKey::F3:
                tabber.GetWindow(2)->SetFocus();
                break;

            case SlokedControlKey::F4:
                win1->SetFocus();
                break;

            case SlokedControlKey::F5:
                win2->SetFocus();
                break;

            default:
                return false;
        }
        return true;
    });

    while (true) {
        screen.UpdateDimensions();
        console.SetGraphicsMode(SlokedTextGraphics::Off);
        console.ClearScreen();
        screen.Render();
        console.Flush();
        auto input = terminal.GetInput();
        for (const auto &evt : input) {
            screen.ProcessInput(evt);
        }
    }

    return EXIT_SUCCESS;
}
