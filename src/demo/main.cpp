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
#include "sloked/services/DocumentNotify.h"
#include "sloked/services/DocumentSet.h"
#include "sloked/services/Screen.h"
#include "sloked/services/ScreenInput.h"
#include "sloked/services/TextPane.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"
#include "sloked/screen/components/ComponentTree.h"
#include "sloked/core/Monitor.h"
#include "sloked/net/PosixSocket.h"
#include "sloked/kgr/net/MasterServer.h"
#include "sloked/kgr/net/SlaveServer.h"
#include "sloked/core/Logger.h"
#include "sloked/core/CLI.h"
#include "sloked/editor/Configuration.h"
#include "sloked/kgr/Path.h"
#include "sloked/core/Semaphore.h"
#include "sloked/sched/Scheduler.h"
#include "sloked/net/Poll.h"
#include "sloked/kgr/net/Config.h"
#include <chrono>

using namespace sloked;
using namespace std::chrono_literals;

class TestFragment : public SlokedTextTagger<int> {
 public:
    TestFragment(const TextBlockView &text, const Encoding &encoding)
        : text(text), encoding(encoding), current{0, 0} {}

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
    TextPosition current;
    std::queue<TaggedTextFragment<int>> cache;
};

class TestFragmentFactory : public SlokedTextTaggerFactory<int> {
 public:
    std::unique_ptr<SlokedTextTagger<int>> Create(const TextBlockView &text, const Encoding &encoding, const SlokedCharWidth &charWidth) const override {
        return std::make_unique<TestFragment>(text, encoding);
    }
};

static const KgrDictionary DefaultConfiguration {
    { "encoding", "system" },
    { "newline", "system" },
    {
        "network", KgrDictionary {
            { "port", 1234 },
        }
    }
};

int main(int argc, const char **argv) {
    SlokedXdgConfiguration mainConfig("main", DefaultConfiguration);
    SlokedCLI cli;
    cli.Define("--encoding", mainConfig.Find("/encoding").AsString());
    cli.Define("--newline", mainConfig.Find("/newline").AsString());
    cli.Define("-o,--output", cli.Option<std::string>());
    cli.Define("--net-port", mainConfig.Find("/network/port").AsInt());
    cli.Parse(argc, argv);
    if (cli.Size() == 0) {
        std::cout << "Format: " << argv[0] << " source -o destination [options]" << std::endl;
        return EXIT_FAILURE;
    }

    SlokedLoggingManager::Global.SetSink(SlokedLogLevel::Warning, SlokedLoggingSink::TextFile("./sloked.log", SlokedLoggingSink::TabularFormat(10, 30, 30)));
    SlokedLogger logger(SlokedLoggerTag);

    logger.Debug() << "Initialization";

    KgrLocalServer portServer;
    KgrLocalNamedServer server(portServer);
    KgrLocalServer portLocalServer;
    KgrLocalNamedServer localServer(portLocalServer);
    KgrRunnableContextManagerHandle<KgrLocalContext> ctxManagerHandle;
    KgrContextManager<KgrLocalContext> &ctxManager = ctxManagerHandle.GetManager();
    ctxManagerHandle.Start();

    SlokedDefaultSchedulerThread sched;
    sched.Start();

    logger.Debug() << "Local servers started";

    SlokedPosixSocketFactory socketFactory;
    auto socketPoll = socketFactory.Poll();

    SlokedDefaultSocketPollThread socketPoller(*socketPoll);
    socketPoller.Start(KgrNetConfig::RequestTimeout);
    KgrMasterNetServer masterServer(server, socketFactory.Bind("localhost", cli["net-port"].As<int>()), socketPoller);
    masterServer.Start();
    KgrSlaveNetServer slaveServer(socketFactory.Connect("localhost", cli["net-port"].As<int>()), localServer, socketPoller);
    slaveServer.Start();

    logger.Debug() << "Network servers started";

    char INPUT_PATH[1024], OUTPUT_PATH[1024];
    realpath(std::string{cli.At(0)}.data(), INPUT_PATH);
    realpath(cli["output"].As<std::string>().data(), OUTPUT_PATH);
    SlokedVirtualNamespace root(std::make_unique<SlokedFilesystemNamespace>(std::make_unique<SlokedPosixFilesystemAdapter>("/")));
    SlokedEditorDocumentSet documents(root);

    logger.Debug() << "Namespaces and documents initialized";

    SlokedLocale::Setup();
    const Encoding &terminalEncoding = Encoding::Get("system");

    SlokedCharWidth charWidth;
    TestFragmentFactory fragmentFactory;

    logger.Debug() << "Misc. initialization";

    PosixTerminal terminal;
    BufferedTerminal console(terminal, terminalEncoding, charWidth);

    TerminalComponentHandle screen(console, terminalEncoding, charWidth);
    SlokedMonitor<SlokedScreenComponent &> screenHandle(screen);

    logger.Debug() << "Screen initialized";

    server.Register("text::render", std::make_unique<SlokedTextRenderService>(documents, charWidth, fragmentFactory, ctxManager));
    server.Register("text::cursor", std::make_unique<SlokedCursorService>(documents, server.GetConnector("text::render"), ctxManager));
    server.Register("documents", std::make_unique<SlokedDocumentSetService>(documents, ctxManager));
    server.Register("document::notify", std::make_unique<SlokedDocumentNotifyService>(documents, ctxManager));
    server.Register("screen", std::make_unique<SlokedScreenService>(screenHandle, terminalEncoding, slaveServer.GetConnector("text::cursor"), slaveServer.GetConnector("document::notify"), ctxManager));
    server.Register("screen::input", std::make_unique<SlokedScreenInputService>(screenHandle, terminalEncoding, ctxManager));
    server.Register("screen::text::pane", std::make_unique<SlokedTextPaneService>(screenHandle, terminalEncoding, ctxManager));

    logger.Debug() << "Services bound";


    SlokedScreenClient screenClient(slaveServer.Connect("screen"));
    SlokedDocumentSetClient documentClient(slaveServer.Connect("documents"));
    documentClient.Open(INPUT_PATH, cli["encoding"].As<std::string>(), cli["newline"].As<std::string>());

    screenClient.Handle.NewMultiplexer("/");
    auto mainWindow = screenClient.Multiplexer.NewWindow("/", TextPosition{0, 0}, TextPosition{console.GetHeight(), console.GetWidth()});
    screenClient.Handle.NewSplitter(mainWindow.value(), Splitter::Direction::Vertical);
    screenClient.Splitter.NewWindow(mainWindow.value(), Splitter::Constraints(1.0f));
    auto tabber = screenClient.Splitter.NewWindow(mainWindow.value(), Splitter::Constraints(0.0f, 1));
    screenClient.Handle.NewTabber("/0/0");
    auto tab1 = screenClient.Tabber.NewWindow("/0/0");
    screenClient.Handle.NewTextEditor(tab1.value(), documentClient.GetId().value());

    SlokedScreenInputClient screenInput(server.Connect("screen::input"));
    screenInput.Connect("/", false, {
        { SlokedControlKey::Escape, false }
    });

    SlokedTextPaneClient paneClient(server.Connect("screen::text::pane"));
    paneClient.Connect("/0/1", false, {});
    auto &render = paneClient.GetRender();

    logger.Debug() << "Editor initialized";

    std::atomic<bool> work = true;
    SlokedSemaphore renderTrigger;
    std::thread renderThread([&] {
        while (work.load()) {
            screenHandle.Lock([&](auto &screen) {
                screen.UpdateDimensions();
                console.SetGraphicsMode(SlokedTextGraphics::Off);
                console.ClearScreen();
                screen.Render();
                console.Flush();
            });
            std::this_thread::sleep_for(25ms);
            renderTrigger.WaitAll();
        }
    });
    screenHandle.Lock([&](auto &screen) {
        screen.OnUpdate([&] {
            renderTrigger.Notify();
        });
    });
    int i = 0;
    while (work.load()) {
        render.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
        render.ClearScreen();
        render.SetPosition(0, 0);
        render.Write(std::to_string(i++));
        render.Flush();
        auto input = terminal.GetInput();
        for (const auto &evt : input) {
            screenHandle.Lock([&](auto &screen) {
                screen.ProcessInput(evt);
            });
        }
        auto closeEvents = screenInput.GetInput();
        if (!closeEvents.empty()) {
            logger.Debug() << "Saving document";
            documentClient.Save(OUTPUT_PATH);
            work = false;
        }
    }
    sched.Stop();
    socketPoller.Stop();
    renderTrigger.Notify();
    renderThread.join();

    logger.Debug() << "Stopping servers";

    sched.Stop();
    slaveServer.Stop();
    masterServer.Stop();
    ctxManagerHandle.Stop();

    logger.Debug() << "Shutdown";
    return EXIT_SUCCESS;
}