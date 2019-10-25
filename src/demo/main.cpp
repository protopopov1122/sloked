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
#include "sloked/screen/terminal/posix/PosixTerminal.h"
#include "sloked/screen/terminal/multiplexer/TerminalBuffer.h"
#include "sloked/screen/terminal/components/ComponentHandle.h"
#include "sloked/screen/terminal/components/SplitterComponent.h"
#include "sloked/screen/terminal/components/TabberComponent.h"
#include "sloked/screen/terminal/components/MultiplexerComponent.h"
#include "sloked/screen/widgets/TextEditor.h"
#include "sloked/namespace/Filesystem.h"
#include "sloked/namespace/Virtual.h"
#include "sloked/namespace/posix/Filesystem.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/kgr/local/Pipe.h"
#include "sloked/kgr/local/Server.h"
#include "sloked/kgr/local/NamedServer.h"
#include "sloked/kgr/ctx-manager/RunnableContextManager.h"
#include "sloked/services/TextRender.h"
#include "sloked/services/Cursor.h"
#include "sloked/services/DocumentSet.h"
#include "sloked/services/Screen.h"
#include "sloked/services/ScreenInput.h"
#include "sloked/services/TextPane.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"
#include "sloked/editor/Tabs.h"
#include "sloked/screen/components/ComponentTree.h"
#include "sloked/core/Synchronized.h"
#include "sloked/kgr/net/PosixSocket.h"
// #include <cstring>

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

    const TextPosition &GetPosition() const override {
        return this->current;
    }

 private:
    bool ParseLine() {
        if (this->current.line > this->text.GetLastLine()) {
            return false;
        }

        std::string currentLine{this->text.GetLine(this->current.line)};
        currentLine.erase(0, this->encoding.GetCodepoint(currentLine, this->current.column).first);
        this->encoding.IterateCodepoints(currentLine, [&](auto start, auto length, auto chr) {
            if (chr == U'\t') {
                this->cache.push(TaggedTextFragment<int>(this->current, TextPosition{0, static_cast<TextPosition::Column>(1)}, 1));
            }
            this->current.column++;
            return true;
        });

        this->current.line++;
        this->current.column = 0;
        return true;
    }

    const TextBlockView &text;
    const Encoding &encoding;
    const SlokedCharWidth &charWidth;
    TextPosition current;
    std::queue<TaggedTextFragment<int>> cache;
};

class TestFragmentFactory : public SlokedTextTaggerFactory<int> {
 public:
    std::unique_ptr<SlokedTextTagger<int>> Create(const TextBlockView &text, const Encoding &encoding, const SlokedCharWidth &charWidth) const override {
        return std::make_unique<TestFragment>(text, encoding, charWidth);
    }
};

int main(int argc, const char **argv) {
    if (argc < 3) {
        std::cout << "Format: " << argv[0] << " source destination" << std::endl;
        return EXIT_FAILURE;
    }

    // KgrPosixSocketFactory factory;
    // auto srv = factory.Bind("localhost", 1235);
    // srv->Start();
    // std::thread([&] {
    //     std::unique_ptr<KgrSocket> socket;
    //     while ((socket = srv->Accept()) != nullptr) {
    //         std::thread([socket = std::move(socket)]() {
    //             std::cout << "Connect" << std::endl;
    //             while (true) {
    //                 auto res = socket->Read(1);
    //                 if (res.empty()) {
    //                     break;
    //                 }
    //                 std::cout << static_cast<int>(res.at(0)) << std::endl;
    //             }
    //             std::cout << "Disconnect" << std::endl;
    //         }).detach();
    //     }
    //     srv->Close();
    // }).detach();
    // auto client = factory.Connect("localhost", 1235);
    // const char *STR = "Hello, world!";
    // client->Write(reinterpret_cast<const uint8_t *>(STR), strlen(STR) + 1);
    // client->Close();
    // while (true) {}
    // return EXIT_SUCCESS;

    KgrLocalServer portServer;
    KgrLocalNamedServer server(portServer);
    KgrLocalServer portScreenServer;
    KgrLocalNamedServer screenServer(portScreenServer);
    KgrRunnableContextManagerHandle<KgrLocalContext> ctxManagerHandle;
    KgrContextManager<KgrLocalContext> &ctxManager = ctxManagerHandle.GetManager();
    ctxManagerHandle.Start();
    KgrRunnableContextManagerHandle<KgrLocalContext> ctxScreenManagerHandle;
    KgrContextManager<KgrLocalContext> &ctxScreenManager = ctxScreenManagerHandle.GetManager();
    ctxScreenManagerHandle.Start();

    char INPUT_PATH[1024], OUTPUT_PATH[1024];
    realpath(argv[1], INPUT_PATH);
    realpath(argv[2], OUTPUT_PATH);
    SlokedVirtualNamespace root(std::make_unique<SlokedFilesystemNamespace>(std::make_unique<SlokedPosixFilesystemAdapter>("/")));
    SlokedEditorDocumentSet documents(root);

    SlokedLocale::Setup();
    const Encoding &terminalEncoding = Encoding::Get("system");

    SlokedCharWidth charWidth;
    TestFragmentFactory fragmentFactory;

    PosixTerminal terminal;
    BufferedTerminal console(terminal, terminalEncoding, charWidth);

    TerminalComponentHandle screen(console, terminalEncoding, charWidth);
    SlokedSynchronized<SlokedScreenComponent &> screenHandle(screen);

    server.Register("text::render", std::make_unique<SlokedTextRenderService>(documents, charWidth, fragmentFactory, ctxManager));
    server.Register("text::cursor", std::make_unique<SlokedCursorService>(documents, ctxManager));
    server.Register("documents", std::make_unique<SlokedDocumentSetService>(documents, ctxManager));
    screenServer.Register("screen", std::make_unique<SlokedScreenService>(screenHandle, terminalEncoding, server.GetConnector("text::cursor"), server.GetConnector("text::render"), ctxScreenManager));
    screenServer.Register("screen::input", std::make_unique<SlokedScreenInputService>(screenHandle, terminalEncoding, ctxScreenManager));
    screenServer.Register("screen::text::pane", std::make_unique<SlokedTextPaneService>(screenHandle, terminalEncoding, ctxScreenManager));

    SlokedScreenClient screenClient(screenServer.Connect("screen"));
    SlokedDocumentSetClient documentClient(server.Connect("documents"));
    documentClient.Open(INPUT_PATH, "system", "system");

    screenClient.Handle.NewMultiplexer("/");
    auto mainWindow = screenClient.Multiplexer.NewWindow("/", TextPosition{0, 0}, TextPosition{console.GetHeight(), console.GetWidth()});
    screenClient.Handle.NewSplitter(mainWindow.value(), Splitter::Direction::Vertical);
    screenClient.Splitter.NewWindow(mainWindow.value(), Splitter::Constraints(1.0f));
    auto tabber = screenClient.Splitter.NewWindow(mainWindow.value(), Splitter::Constraints(0.0f, 1));
    screenClient.Handle.NewTabber("/0/0");
    auto tab1 = screenClient.Tabber.NewWindow("/0/0");
    screenClient.Handle.NewTextEditor(tab1.value(), documentClient.GetId().value());
    // SlokedEditorTabs tabs(SlokedComponentTree::Traverse(screen, "/0/0/self").AsTabber(), terminalEncoding, server.GetConnector("text::cursor"), server.GetConnector("text::render"), server.GetConnector("documents"));

    // auto tab = tabs.Open(INPUT_PATH, "system", "system");

    SlokedScreenInputClient screenInput(screenServer.Connect("screen::input"));
    screenInput.Connect("/", false, {
        { SlokedControlKey::Escape, false }
    });

    SlokedTextPaneClient paneClient(screenServer.Connect("screen::text::pane"));
    paneClient.Connect("/0/1", false, {});
    auto &render = paneClient.GetRender();

    int i = 0;
    bool work = true;
    while (work) {
        render.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
        render.ClearScreen();
        render.SetPosition(0, 0);
        render.Write(std::to_string(i++));
        render.Flush();
        screenHandle.Lock([&](auto &screen) {
            screen.UpdateDimensions();
            console.SetGraphicsMode(SlokedTextGraphics::Off);
            console.ClearScreen();
            screen.Render();
            console.Flush();
        });
        auto input = terminal.GetInput();
        for (const auto &evt : input) {
            screenHandle.Lock([&](auto &screen) {
                screen.ProcessInput(evt);
            });
        }
        auto closeEvents = screenInput.GetInput();
        if (!closeEvents.empty()) {
            documentClient.Save(OUTPUT_PATH);
            work = false;
        }
    }
    // tabs.CloseAll();

    ctxScreenManagerHandle.Stop();
    ctxManagerHandle.Stop();
    return EXIT_SUCCESS;
}