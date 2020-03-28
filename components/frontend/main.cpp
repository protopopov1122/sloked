/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <GL/glu.h>

#include <chrono>
#include <cstdlib>
#include <iostream>

#include "sloked/compression/Compat.h"
#include "sloked/core/CLI.h"
#include "sloked/core/Closeable.h"
#include "sloked/core/Failure.h"
#include "sloked/core/Locale.h"
#include "sloked/core/Logger.h"
#include "sloked/core/Monitor.h"
#include "sloked/core/Semaphore.h"
#include "sloked/core/awaitable/Compat.h"
#include "sloked/core/awaitable/Poll.h"
#include "sloked/crypto/Compat.h"
#include "sloked/editor/Configuration.h"
#include "sloked/editor/EditorInstance.h"
#include "sloked/editor/Manager.h"
#include "sloked/editor/configuration/Compat.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"
#include "sloked/editor/terminal/ScreenProvider.h"
#include "sloked/facade/Services.h"
#include "sloked/kgr/Path.h"
#include "sloked/kgr/ctx-manager/RunnableContextManager.h"
#include "sloked/kgr/local/NamedServer.h"
#include "sloked/kgr/local/Pipe.h"
#include "sloked/kgr/local/Server.h"
#include "sloked/kgr/net/Config.h"
#include "sloked/kgr/net/MasterServer.h"
#include "sloked/kgr/net/SlaveServer.h"
#include "sloked/namespace/Compat.h"
#include "sloked/namespace/Empty.h"
#include "sloked/namespace/Mount.h"
#include "sloked/namespace/Resolve.h"
#include "sloked/namespace/Root.h"
#include "sloked/namespace/Virtual.h"
#include "sloked/net/Compat.h"
#include "sloked/net/CryptoSocket.h"
#include "sloked/sched/Scheduler.h"
#include "sloked/screen/graphics/Compat.h"
#include "sloked/screen/terminal/Compat.h"
#include "sloked/screen/terminal/TerminalResize.h"
#include "sloked/screen/terminal/TerminalSize.h"
#include "sloked/screen/terminal/multiplexer/TerminalBuffer.h"
#include "sloked/script/Compat.h"
#include "sloked/security/Master.h"
#include "sloked/security/Slave.h"
#include "sloked/services/Cursor.h"
#include "sloked/services/DocumentNotify.h"
#include "sloked/services/DocumentSet.h"
#include "sloked/services/Namespace.h"
#include "sloked/services/Screen.h"
#include "sloked/services/ScreenInput.h"
#include "sloked/services/ScreenSize.h"
#include "sloked/services/Search.h"
#include "sloked/services/TextPane.h"
#include "sloked/services/TextRender.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/text/fragment/Updater.h"

using namespace sloked;

class TestFragment : public SlokedTextTagIterator<int> {
    class DocumentListener : public SlokedTransactionStreamListener {
     public:
        DocumentListener(SlokedEventEmitter<const TextPositionRange &> &emitter)
            : emitter(emitter) {}

        void OnCommit(const SlokedCursorTransaction &transaction) final {
            this->Emit(transaction.GetPosition().line);
        }

        void OnRollback(const SlokedCursorTransaction &transaction) final {
            this->Emit(transaction.GetPosition().line);
        }

        void OnRevert(const SlokedCursorTransaction &transaction) final {
            this->Emit(transaction.GetPosition().line);
        }

     private:
        void Emit(TextPosition::Line line) {
            if (line > 0) {
                emitter.Emit({TextPosition{line - 1, 0}, TextPosition::Max});
            } else {
                emitter.Emit({TextPosition{0, 0}, TextPosition::Max});
            }
        }

        SlokedEventEmitter<const TextPositionRange &> &emitter;
    };

 public:
    TestFragment(const TextBlockView &text, const Encoding &encoding,
                 SlokedTransactionListenerManager &listeners)
        : text(text), encoding(encoding), current{0, 0}, listeners(listeners) {
        this->listener = std::make_shared<DocumentListener>(this->emitter);
        this->listeners.AddListener(this->listener);
    }

    ~TestFragment() {
        this->listeners.RemoveListener(*this->listener);
    }

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
            this->emitter.Emit(TextPositionRange{position, TextPosition::Max});
        }
    }

    const TextPosition &GetPosition() const override {
        return this->current;
    }

    Unbind OnChange(std::function<void(const TextPositionRange &)> fn) final {
        return this->emitter.Listen(std::move(fn));
    }

 private:
    bool ParseLine() {
        if (this->current.line > this->text.GetLastLine()) {
            return false;
        }

        std::string currentLine{this->text.GetLine(this->current.line)};
        currentLine.erase(
            0, this->encoding.GetCodepoint(currentLine, this->current.column)
                   .first);
        this->encoding.IterateCodepoints(
            currentLine, [&](auto start, auto length, auto chr) {
                if (chr == U'\t') {
                    this->cache.push(TaggedTextFragment<int>(
                        this->current,
                        TextPosition{0, static_cast<TextPosition::Column>(1)},
                        1));
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
    SlokedEventEmitter<const TextPositionRange &> emitter;
    SlokedTransactionListenerManager &listeners;
    std::shared_ptr<DocumentListener> listener;
};

class SlokedTestTagger : public SlokedTextTagger<int> {
 public:
    SlokedTestTagger(SlokedTaggableDocument &doc)
        : doc(doc),
          iter(doc.GetText(), doc.GetEncoding(), doc.GetTransactionListeners()),
          updater(std::make_shared<SlokedFragmentUpdater<int>>(
              doc.GetText(), iter, doc.GetEncoding())),
          lazy(iter), cached(lazy) {
        doc.GetTransactionListeners().AddListener(this->updater);
    }

    ~SlokedTestTagger() {
        doc.GetTransactionListeners().RemoveListener(*this->updater);
    }

    std::optional<TaggedTextFragment<int>> Get(const TextPosition &pos) final {
        return this->cached.Get(pos);
    }

    std::vector<TaggedTextFragment<int>> Get(TextPosition::Line line) final {
        return this->cached.Get(line);
    }

    typename SlokedTextTagger<int>::Unbind OnChange(
        std::function<void(const TextPositionRange &)> callback) final {
        return this->cached.OnChange(std::move(callback));
    }

 private:
    SlokedTaggableDocument &doc;
    TestFragment iter;
    std::shared_ptr<SlokedFragmentUpdater<int>> updater;
    SlokedLazyTaggedText<int> lazy;
    SlokedCacheTaggedText<int> cached;
};

class TestFragmentFactory : public SlokedTextTaggerFactory<int> {
 public:
    std::unique_ptr<SlokedTextTagger<int>> Create(
        SlokedTaggableDocument &doc) const final {
        return std::make_unique<SlokedTestTagger>(doc);
    }
};

class SlokedDemoSystemNamespace : public SlokedRootNamespace {
 public:
    SlokedDemoSystemNamespace()
        : root(std::make_unique<SlokedEmptyNamespace>()),
          mounter(SlokedNamespaceCompat::NewRootFilesystem(), root),
          resolver(SlokedNamespaceCompat::GetWorkDir(),
                   SlokedNamespaceCompat::GetHomeDir()) {}

    SlokedPathResolver &GetResolver() final {
        return this->resolver;
    }

    SlokedMountableNamespace &GetRoot() final {
        return this->root;
    }

    SlokedNamespaceMounter &GetMounter() final {
        return this->mounter;
    }

 private:
    SlokedDefaultVirtualNamespace root;
    SlokedDefaultNamespaceMounter mounter;
    SlokedPathResolver resolver;
};

class SlokedDemoRootNamespaceFactory : public SlokedRootNamespaceFactory {
 public:
    std::unique_ptr<SlokedRootNamespace> Build() const final {
        return std::make_unique<SlokedDemoSystemNamespace>();
    }
};

class SlokedGraphicalScreenBasis : public SlokedScreenProvider {
 public:
    struct GUI {
        GUI(int width, int height)
            : gui(SlokedGraphicsCompat::GetGraphics(this->screenMgr)),
              terminal(gui->OpenTerminal({{width, height}, "Monospace 10"})) {
            this->screenMgr.Start(std::chrono::milliseconds(50));
        }

        ~GUI() {
            this->screenMgr.Stop();
            this->terminal->Close();
        }

        SlokedGraphicalTerminal &GetTerminal() {
            return this->terminal->GetTerminal();
        }

        SlokedScreenManager screenMgr;
        std::unique_ptr<SlokedGraphicalComponents> gui;
        std::unique_ptr<SlokedGraphicalTerminalWindow> terminal;
    };

    SlokedGraphicalScreenBasis(const SlokedCharPreset &charPreset)
        : gui(1024, 960),
          console(gui.GetTerminal(), Encoding::Get("system"), charPreset),
          provider(console, Encoding::Get("system"), charPreset,
                   gui.GetTerminal(), gui.GetTerminal().GetTerminalSize()) {}

    void Render(std::function<void(SlokedScreenComponent &)> fn) final {
        this->provider.Render(std::move(fn));
    }

    std::vector<SlokedKeyboardInput> ReceiveInput(
        std::chrono::system_clock::duration timeout) final {
        return this->provider.ReceiveInput(timeout);
    }

    SlokedMonitor<SlokedScreenComponent &> &GetScreen() final {
        return this->provider.GetScreen();
    }

    SlokedScreenSize &GetSize() final {
        return this->provider.GetSize();
    }

    const Encoding &GetEncoding() final {
        return this->provider.GetEncoding();
    }

 private:
    GUI gui;
    BufferedTerminal console;
    SlokedTerminalScreenProvider provider;
};

class SlokedConsoleScreenBasis : public SlokedScreenProvider {
 public:
    SlokedConsoleScreenBasis(const SlokedCharPreset &charPreset)
        : terminal(*SlokedTerminalCompat::GetSystemTerminal()), size(terminal),
          console(terminal, Encoding::Get("system"), charPreset),
          provider(console, Encoding::Get("system"), charPreset, terminal,
                   size) {}

    void Render(std::function<void(SlokedScreenComponent &)> fn) final {
        this->provider.Render(std::move(fn));
    }

    std::vector<SlokedKeyboardInput> ReceiveInput(
        std::chrono::system_clock::duration timeout) final {
        return this->provider.ReceiveInput(timeout);
    }

    SlokedMonitor<SlokedScreenComponent &> &GetScreen() final {
        return this->provider.GetScreen();
    }

    SlokedScreenSize &GetSize() final {
        return this->provider.GetSize();
    }

    const Encoding &GetEncoding() final {
        return this->provider.GetEncoding();
    }

 private:
    SlokedDuplexTerminal &terminal;
    SlokedTerminalSize<SlokedTerminalResizeListener> size;
    BufferedTerminal console;
    SlokedTerminalScreenProvider provider;
};

class SlokedDemoScreenFactory : public SlokedScreenProviderFactory {
 public:
    std::unique_ptr<SlokedScreenProvider> Make(
        const SlokedUri &, const SlokedCharPreset &charPreset) final {
        if constexpr (SlokedGraphicsCompat::HasGraphics()) {
            return std::make_unique<SlokedGraphicalScreenBasis>(charPreset);
        } else {
            return std::make_unique<SlokedConsoleScreenBasis>(charPreset);
        }
    }
};

static const KgrValue DefaultConfiguration =
    KgrDictionary{{"encoding", "system"},
                  {"newline", "system"},
                  {"network", KgrDictionary{
                                  {"port", 1234},
                              }}};

class SlokedTestCharVisualPreset : public SlokedFontProperties {
 public:
    SlokedGraphicsPoint::Coordinate GetHeight() const final {
        return 1;
    }

    SlokedGraphicsPoint::Coordinate GetWidth(char32_t chr) const final {
        return chr == U' ' ? 2 : 1;
    }
};

int main(int argc, const char **argv) {
    // Initialize globals
    SlokedFailure::SetupHandler();
    SlokedLocale::Setup();
    SlokedLoggingManager::Global.SetSink(
        SlokedLogLevel::Debug,
        SlokedLoggingSink::TextFile(
            "./sloked.log", SlokedLoggingSink::TabularFormat(10, 30, 30)));
    SlokedLogger logger(SlokedLoggerTag);
    const Encoding &terminalEncoding = Encoding::Get("system");
    SlokedCloseablePool closeables;
    SlokedDemoRootNamespaceFactory nsFactory;
    SlokedDefaultTextTaggerRegistry<int> baseTaggers;
    baseTaggers.Bind("default", std::make_unique<TestFragmentFactory>());
    SlokedEditorManager::Parameters startupPrms(logger, nsFactory);
    startupPrms.SetTaggers(baseTaggers);
    if constexpr (SlokedCryptoCompat::IsSupported()) {
        startupPrms.SetCrypto(SlokedCryptoCompat::GetCrypto());
    }
    startupPrms.SetEditors([] {
        return std::make_unique<SlokedEditorInstance>(
            SlokedIOPollCompat::NewPoll(), SlokedNetCompat::GetNetwork());
    });
    if constexpr (!SlokedTerminalCompat::HasSystemTerminal()) {
        throw SlokedError("Demo: system terminal is required");
    }

    if constexpr (SlokedCompressionCompat::IsSupported()) {
        startupPrms.SetComresssion(SlokedCompressionCompat::GetCompression());
    }
    SlokedDemoScreenFactory screenFactory;
    startupPrms.SetScreenProviders(screenFactory);
    SlokedEditorManager startup(std::move(startupPrms));
    closeables.Attach(startup);

    // Configuration
    SlokedCLI cli;
    cli.Initialize(KgrArray{
        KgrDictionary{{"options", "--encoding"},
                      {"type", "string"},
                      {"map", "/encoding"}},
        KgrDictionary{
            {"options", "--newline"}, {"type", "string"}, {"map", "/newline"}},
        KgrDictionary{{"options", "-o,--output"},
                      {"type", "string"},
                      {"mandatory", true},
                      {"map", "/output"}},
        KgrDictionary{{"options", "--net-port"},
                      {"type", "int"},
                      {"map", "/network{}/port"}},
        KgrDictionary{{"options", "--script"},
                      {"type", "string"},
                      {"map", "/script{}/init"}},
        KgrDictionary{{"options", "--script-path"},
                      {"type", "string"},
                      {"map", "/script{}/path"}}});
    cli.Parse(argc, argv);
    SlokedConfigurationLoader &mainConfigLoader =
        SlokedConfigurationLoaderCompat::GetLoader();
    SlokedConfiguration mainConfig{cli.Export(), mainConfigLoader.Load("main"),
                                   DefaultConfiguration};
    if (cli.ArgCount() == 0) {
        std::cout << "Format: " << argv[0] << " source -o destination [options]"
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Main editor
    KgrDictionary mainEditorConfig{
        {"crypto",
         KgrDictionary{
             {"masterPassword", "password"},
             {"salt", "salt"},
             {"authentication",
              KgrDictionary{
                  {"master",
                   KgrDictionary{
                       {"masterPassword", "password"},
                       {"salt", "salt"},
                       {"users",
                        KgrArray{KgrDictionary{
                            {"id", "user1"},
                            {"restrictAccess",
                             KgrDictionary{
                                 {"whitelist", true},
                                 {"content", KgrArray{"/document", "/namespace",
                                                      "/screen", "/editor"}}}},
                            {"restrictModification",
                             KgrDictionary{
                                 {"whitelist", true},
                                 {"content", KgrArray{"/screen"}}}}}}},
                       {"defaultUser",
                        KgrDictionary{
                            {"restrictAccess",
                             KgrDictionary{{"whitelist", true},
                                           {"content", KgrArray{}}}},
                            {"restrictModification",
                             KgrDictionary{{"whitelist", true},
                                           {"content", KgrArray{}}}}}}}}}}}},
        {"network", KgrDictionary{{"buffering", 5},
                                  {"compression",
                                   SlokedCompressionCompat::IsSupported()}}},
        {"server",
         KgrDictionary{
             {"netServer",
              KgrDictionary{
                  {"host", "localhost"},
                  {"port", mainConfig.Find("/network/port").AsInt()}}},
             {"restrictAccess",
              KgrDictionary{{"whitelist", true},
                            {"content", KgrArray{"/document", "/namespace",
                                                 "/screen", "/editor"}}}},
             {"restrictModification",
              KgrDictionary{{"whitelist", true},
                            {"content", KgrArray{"/document", "/namespace",
                                                 "/screen", "/editor"}}}},
             {"services",
              KgrDictionary{{"root", "file:///"},
                            {"endpoints",
                             KgrArray{"/document/render", "/document/cursor",
                                      "/document/manager", "/document/notify",
                                      "/document/search", "/namespace/root",
                                      "/editor/parameters"}}}}}},
        {"parameters", KgrDictionary{{"tabWidth", 2}}}};
    auto &mainEditor = startup.Spawn("main", mainEditorConfig);

    KgrDictionary secondaryEditorConfig{
        {"network", KgrDictionary{{"buffering", 5},
                                  {"compression",
                                   SlokedCompressionCompat::IsSupported()}}},
        {"server",
         KgrDictionary{
             {"slave",
              KgrDictionary{
                  {"address",
                   KgrDictionary{
                       {"host", "localhost"},
                       {"port", mainConfig.Find("/network/port").AsInt()}}}}},
             {"screen", "terminal:///system"}}}};

    if constexpr (SlokedCryptoCompat::IsSupported()) {
        secondaryEditorConfig["server"]
            .AsDictionary()["slave"]
            .AsDictionary()
            .Put("authorize", "user1");
        secondaryEditorConfig.Put(
            "crypto",
            KgrDictionary{
                {"masterPassword", "password"},
                {"salt", "salt"},
                {"authentication",
                 KgrDictionary{
                     {"slave",
                      KgrDictionary{
                          {"users",
                           KgrArray{KgrDictionary{
                               {"id", "user1"},
                               {"credentials", mainEditor.GetCrypto()
                                                   .GetCredentialMaster()
                                                   .GetAccountByName("user1")
                                                   .lock()
                                                   ->GetCredentials()}}}}}}}}});
    }

    auto &secondaryEditor = startup.Spawn("secondary", secondaryEditorConfig);
    auto &secondaryServer = secondaryEditor.GetServer();

    auto &serviceProvider = mainEditor.GetServiceProvider();
    SlokedPath inputPath = serviceProvider.GetNamespace().GetResolver().Resolve(
        SlokedPath{cli.Argument(0)});
    SlokedPath outputPath =
        serviceProvider.GetNamespace().GetResolver().Resolve(
            SlokedPath{mainConfig.Find("/output").AsString()});

    // Screen
    auto &screenServer = secondaryEditor.GetScreen();

    // Editor initialization
    auto isScreenLocked = [&screenServer] {
        return screenServer.GetScreen().GetScreen().IsHolder();
    };
    SlokedScreenClient screenClient(
        secondaryServer.GetServer().Connect({"/screen/manager"}),
        isScreenLocked);
    SlokedScreenSizeNotificationClient screenSizeClient(
        secondaryServer.GetServer().Connect({"/screen/size/notify"}));
    SlokedDocumentSetClient documentClient(
        secondaryServer.GetServer().Connect({"/document/manager"}));
    documentClient.Open(inputPath.ToString(),
                        mainConfig.Find("/encoding").AsString(),
                        mainConfig.Find("/newline").AsString(), "default");

    // Screen layout
    screenClient.Handle.NewMultiplexer("/");
    auto mainWindow = screenClient.Multiplexer.NewWindow(
        "/", TextPosition{0, 0},
        TextPosition{
            screenServer.GetScreen().GetSize().GetScreenSize().line,
            screenServer.GetScreen().GetSize().GetScreenSize().column});
    screenSizeClient.Listen([&](const auto &size) {
        mainEditor.GetThreadManager().Spawn([&, size] {
            if (screenServer.IsRunning() && mainWindow.has_value()) {
                screenClient.Multiplexer.ResizeWindow(mainWindow.value(), size);
            }
        });
    });
    screenClient.Handle.NewSplitter(mainWindow.value(),
                                    Splitter::Direction::Vertical);
    screenClient.Splitter.NewWindow(mainWindow.value(),
                                    Splitter::Constraints(1.0f));
    auto tabber = screenClient.Splitter.NewWindow(
        mainWindow.value(), Splitter::Constraints(0.0f, 1));
    screenClient.Handle.NewTabber("/0/0");
    auto tab1 = screenClient.Tabber.NewWindow("/0/0");
    screenClient.Handle.NewTextEditor(
        tab1.value(), documentClient.GetId().value(), "default");

    SlokedTextPaneClient paneClient(
        secondaryServer.GetServer().Connect({"/screen/component/text/pane"}),
        isScreenLocked);
    paneClient.Connect("/0/1", false, {});
    auto &render = paneClient.GetRender();

    // Startup
    SlokedSemaphore terminate;
    int i = 0;
    auto renderStatus = [&] {
        render.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
        render.SetGraphicsMode(SlokedTextGraphics::Bold);
        render.SetGraphicsMode(SlokedTextGraphics::Underscore);
        render.SetPosition(0, 0);
        render.ClearArea(TextPosition{1, 10});
        render.Write(std::to_string(i++));
        render.Flush();
    };
    renderStatus();
    SlokedScreenInputNotificationClient screenInput(
        secondaryServer.GetServer().Connect({"/screen/component/input/notify"}),
        terminalEncoding, isScreenLocked);
    screenInput.Listen(
        "/",
        [&](auto &evt) {
            mainEditor.GetScheduler().Defer([&, evt] {
                renderStatus();
                if (evt.value.index() != 0 &&
                    std::get<1>(evt.value) == SlokedControlKey::Escape) {
                    logger.Debug() << "Saving document";
                    documentClient.Save(outputPath.ToString());
                    terminate.Notify();
                }
            });
        },
        true);

    // Scripting engine startup
    std::unique_ptr<SlokedScriptEngine> scriptEngine;
    if constexpr (SlokedScriptCompat::IsSupported()) {
        if (mainConfig.Has("/script/init")) {
            scriptEngine = SlokedScriptCompat::GetEngine(
                startup, mainEditor.GetScheduler(),
                mainConfig.Find("/script/path").AsString());
            closeables.Attach(*scriptEngine);
            scriptEngine->Start(mainConfig.Find("/script/init").AsString());
        }
    }
    terminate.WaitAll();
    closeables.Close();
    return EXIT_SUCCESS;
}