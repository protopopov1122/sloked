/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>
#include <iostream>
#include "sloked/core/Locale.h"
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
#include "sloked/kgr/Serialize.h"
#include "sloked/kgr/local/Pipe.h"
#include "sloked/kgr/local/Server.h"
#include "sloked/kgr/local/NamedServer.h"
#include "sloked/kgr/ctx-manager/RunnableContextManager.h"
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <queue>
#include <thread>
#include <chrono>

using namespace sloked;
using namespace std::chrono_literals;

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

        std::string currentLine{this->text.GetLine(this->current.line)};
        auto lineLength = this->encoding.CodepointCount(currentLine);
        while (this->current.column < lineLength) {
            auto position = this->encoding.GetCodepoint(currentLine, this->current.column);
            auto substr = currentLine.substr(position.first, 1);
            if (substr.compare("\t") == 0) {
                this->cache.push(TaggedTextFragment<int>(this->current, TextPosition{0, static_cast<TextPosition::Column>(1)}, 1));
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

class SumServiceContext : public KgrLocalContext {
 public:
    SumServiceContext(int64_t n, std::unique_ptr<KgrPipe> pipe, KgrContextManager<KgrLocalContext> &ctxMgr)
        : KgrLocalContext(std::move(pipe)), number(n) {}

    void Run() override {
        try {
            while (!this->pipe->Empty()) {
                auto msg = this->pipe->Read();
                if (msg.Is(KgrValueType::Array)) {
                    KgrArray result;
                    for (const auto &el : msg.AsArray()) {
                        if (el.Is(KgrValueType::Integer)) {
                            result.Append(el.AsInt() + this->number);
                        }
                    }
                    this->pipe->Write(std::move(result));
                } else {
                    this->pipe->Write(KgrArray {});
                }
            }
        } catch (const SlokedError &ex) {
            if (this->pipe->GetStatus() == KgrPipe::Status::Open) {
                throw;
            }
        }
    }

 private:
    int64_t number;
};

class SumService : public KgrService {
 public:
    SumService(int64_t n, KgrContextManager<KgrLocalContext> &ctxMgr)
        : number(n), ctxMgr(ctxMgr) {}

    bool Attach(std::unique_ptr<KgrPipe> pipe) override {
        auto ctx = std::make_unique<SumServiceContext>(this->number, std::move(pipe), this->ctxMgr);
        this->ctxMgr.Attach(std::move(ctx));
        return true;
    }

 private:
    int64_t number;
    KgrContextManager<KgrLocalContext> &ctxMgr;
};

int main(int argc, const char **argv) {
    if (argc < 3) {
        std::cout << "Format: " << argv[0] << " source destination" << std::endl;
        return EXIT_FAILURE;
    }

    // KgrLocalServer portServer;
    // KgrLocalNamedServer server(portServer);
    // KgrRunnableContextManager<KgrLocalContext> ctxManager;
    // server.Register("sum.10", std::make_unique<SumService>(10, ctxManager));

    // std::atomic<bool> thread_run = true;
    // std::thread ctxMgrThread([&]() {
    //     while (thread_run.load()) {
    //         ctxManager.Run();
    //     }
    // });

    // auto in = server.Connect("sum.10");
    // in->Write(KgrArray {
    //     1,
    //     2,
    //     3,
    //     3.14,
    //     "Hello, world",
    //     5,
    //     true,
    //     KgrDictionary {}
    // });
    // in->Write({});
    // in->Write(KgrArray {3, 2, 1});
    // in->Wait(3);
    // while (!in->Empty()) {
    //     auto val = in->Read();
    //     std::cout << KgrJsonSerializer{}.Serialize(val) << std::endl;
    // }
    // thread_run = false;
    // ctxMgrThread.join();
    // return EXIT_SUCCESS;

    char BUFFER[1024];
    realpath(argv[1], BUFFER);
    SlokedVirtualNamespace root(std::make_unique<SlokedFilesystemNamespace>(std::make_unique<SlokedPosixFilesystemAdapter>("/")));
    auto view = root.GetObject(BUFFER)->AsFile()->View();

    SlokedLocale::Setup();
    const Encoding &fileEncoding = SlokedLocale::SystemEncoding();
    const Encoding &terminalEncoding = SlokedLocale::SystemEncoding();
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