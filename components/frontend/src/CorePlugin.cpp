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

#include "sloked/editor/CorePlugin.h"

#include <iostream>

#include "sloked/core/Semaphore.h"
#include "sloked/services/DocumentSet.h"
#include "sloked/services/Screen.h"
#include "sloked/services/ScreenInput.h"
#include "sloked/services/ScreenSize.h"
#include "sloked/services/TextPane.h"
#include "sloked/text/fragment/Iterator.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/text/fragment/Updater.h"

namespace sloked {

    class SlokedFrontendDefaultCorePlugin : public SlokedCorePlugin {
     public:
        int Start(int, const char **, const SlokedBaseInterface &,
                  SlokedEditorManager &) final;
    };

    class TestFragment
        : public SlokedTextTagIterator<SlokedEditorDocument::TagType> {
        class DocumentListener : public SlokedTransactionStreamListener {
         public:
            DocumentListener(
                SlokedEventEmitter<const TextPositionRange &> &emitter)
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
                    emitter.Emit(
                        {TextPosition{line - 1, 0}, TextPosition::Max});
                } else {
                    emitter.Emit({TextPosition{0, 0}, TextPosition::Max});
                }
            }

            SlokedEventEmitter<const TextPositionRange &> &emitter;
        };

     public:
        TestFragment(const TextBlockView &text, const Encoding &encoding,
                     SlokedTransactionListenerManager &listeners)
            : text(text), encoding(encoding), current{0, 0},
              listeners(listeners) {
            this->listener = std::make_shared<DocumentListener>(this->emitter);
            this->listeners.AddListener(this->listener);
        }

        ~TestFragment() {
            this->listeners.RemoveListener(*this->listener);
        }

        std::optional<TaggedTextFragment<SlokedEditorDocument::TagType>> Next()
            override {
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
                this->emitter.Emit(
                    TextPositionRange{position, TextPosition::Max});
            }
        }

        const TextPosition &GetPosition() const override {
            return this->current;
        }

        Unbind OnChange(
            std::function<void(const TextPositionRange &)> fn) final {
            return this->emitter.Listen(std::move(fn));
        }

     private:
        bool ParseLine() {
            if (this->current.line > this->text.GetLastLine()) {
                return false;
            }

            std::string currentLine{this->text.GetLine(this->current.line)};
            currentLine.erase(
                0,
                this->encoding.GetCodepoint(currentLine, this->current.column)
                    .first);
            this->encoding.IterateCodepoints(
                currentLine, [&](auto start, auto length, auto chr) {
                    if (chr == U'\t') {
                        this->cache.push(
                            TaggedTextFragment<SlokedEditorDocument::TagType>(
                                this->current,
                                TextPosition{
                                    0, static_cast<TextPosition::Column>(1)},
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
        std::queue<TaggedTextFragment<SlokedEditorDocument::TagType>> cache;
        SlokedEventEmitter<const TextPositionRange &> emitter;
        SlokedTransactionListenerManager &listeners;
        std::shared_ptr<DocumentListener> listener;
    };

    class SlokedTestTagger
        : public SlokedTextTagger<SlokedEditorDocument::TagType> {
     public:
        SlokedTestTagger(SlokedTaggableDocument &doc)
            : doc(doc), iter(doc.GetText(), doc.GetEncoding(),
                             doc.GetTransactionListeners()),
              updater(std::make_shared<
                      SlokedFragmentUpdater<SlokedEditorDocument::TagType>>(
                  doc.GetText(), iter, doc.GetEncoding())),
              lazy(iter), cached(lazy) {
            doc.GetTransactionListeners().AddListener(this->updater);
        }

        ~SlokedTestTagger() {
            doc.GetTransactionListeners().RemoveListener(*this->updater);
        }

        std::optional<TaggedTextFragment<SlokedEditorDocument::TagType>> Get(
            const TextPosition &pos) final {
            return this->cached.Get(pos);
        }

        std::vector<TaggedTextFragment<SlokedEditorDocument::TagType>> Get(
            TextPosition::Line line) final {
            return this->cached.Get(line);
        }

        typename SlokedTextTagger<SlokedEditorDocument::TagType>::Unbind
            OnChange(
                std::function<void(const TextPositionRange &)> callback) final {
            return this->cached.OnChange(std::move(callback));
        }

     private:
        SlokedTaggableDocument &doc;
        TestFragment iter;
        std::shared_ptr<SlokedFragmentUpdater<SlokedEditorDocument::TagType>>
            updater;
        SlokedLazyTaggedText<SlokedEditorDocument::TagType> lazy;
        SlokedCacheTaggedText<SlokedEditorDocument::TagType> cached;
    };

    class TestFragmentFactory
        : public SlokedTextTaggerFactory<SlokedEditorDocument::TagType> {
     public:
        std::unique_ptr<SlokedTextTagger<SlokedEditorDocument::TagType>> Create(
            SlokedTaggableDocument &doc) const final {
            return std::make_unique<SlokedTestTagger>(doc);
        }
    };

    static const KgrValue DefaultConfiguration =
        KgrDictionary{{"encoding", "system"},
                      {"newline", "system"},
                      {"network", KgrDictionary{
                                      {"port", 1234},
                                  }}};

    int SlokedFrontendDefaultCorePlugin::Start(
        int argc, const char **argv, const SlokedBaseInterface &baseInterface,
        SlokedEditorManager &manager) {
        SlokedCloseablePool closeables;
        SlokedLoggingManager::Global.SetSink(
            SlokedLogLevel::Debug,
            SlokedLoggingSink::TextFile(
                "./sloked.log", SlokedLoggingSink::TabularFormat(10, 30, 30)));

        SlokedLogger logger(SlokedLoggerTag);
        manager.GetBaseTaggers().Bind("default",
                                      std::make_unique<TestFragmentFactory>());

        // Configuration
        SlokedCLI cli;
        cli.Initialize(KgrArray{KgrDictionary{{"options", "--encoding"},
                                              {"type", "string"},
                                              {"map", "/encoding"}},
                                KgrDictionary{{"options", "--newline"},
                                              {"type", "string"},
                                              {"map", "/newline"}},
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
                                              {"map", "/script{}/path"}},
                                KgrDictionary{{"options", "--core-plugin"},
                                              {"type", "string"},
                                              {"map", "/corePlugin"}}});
        cli.Parse(argc, argv);
        SlokedConfiguration mainConfig{
            cli.Export(), baseInterface.GetConfigurationLoader().Load("main"),
            DefaultConfiguration};
        if (cli.ArgCount() == 0) {
            std::cout << "Format: " << argv[0]
                      << " source -o destination [options]" << std::endl;
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
                                     {"content",
                                      KgrArray{"/document", "/namespace",
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
                                 KgrDictionary{
                                     {"whitelist", true},
                                     {"content", KgrArray{}}}}}}}}}}}},
            {"network",
             KgrDictionary{{"buffering", 5},
                           {"compression", manager.HasCompression()}}},
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
                  KgrDictionary{
                      {"root", "file:///"},
                      {"endpoints",
                       KgrArray{"/document/render", "/document/cursor",
                                "/document/manager", "/document/notify",
                                "/document/search", "/namespace/root",
                                "/editor/parameters"}}}}}},
            {"parameters", KgrDictionary{{"tabWidth", 2}}}};
        auto &mainEditor = manager.Spawn("main", mainEditorConfig);

        KgrDictionary secondaryEditorConfig{
            {"network",
             KgrDictionary{{"buffering", 5},
                           {"compression", manager.HasCompression()}}},
            {"server",
             KgrDictionary{
                 {"slave",
                  KgrDictionary{
                      {"address",
                       KgrDictionary{{"host", "localhost"},
                                     {"port", mainConfig.Find("/network/port")
                                                  .AsInt()}}}}},
                 {"screen", "graphics:///?fallback=terminal"}}}};

        if (manager.HasCrypto()) {
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
                              {"users", KgrArray{KgrDictionary{
                                            {"id", "user1"},
                                            {"credentials",
                                             mainEditor.GetCrypto()
                                                 .GetCredentialMaster()
                                                 .GetAccountByName("user1")
                                                 .lock()
                                                 ->GetCredentials()}}}}}}}}});
        }

        auto &secondaryEditor =
            manager.Spawn("secondary", secondaryEditorConfig);
        auto &secondaryServer = secondaryEditor.GetServer();

        auto &serviceProvider = mainEditor.GetServiceProvider();
        SlokedPath inputPath =
            serviceProvider.GetNamespace().GetResolver().Resolve(
                SlokedPath{cli.Argument(0)});
        SlokedPath outputPath =
            serviceProvider.GetNamespace().GetResolver().Resolve(
                SlokedPath{mainConfig.Find("/output").AsString()});

        // Screen
        auto &screenServer = secondaryEditor.GetScreen();

        // Editor initialization
        SlokedScreenClient screenClient(
            std::move(secondaryServer.GetServer()
                          .Connect({"/screen/manager"})
                          .UnwrapWait()));
        SlokedScreenSizeNotificationClient screenSizeClient;
        screenSizeClient
            .Connect(std::move(secondaryServer.GetServer()
                                   .Connect({"/screen/size/notify"})
                                   .UnwrapWait()))
            .UnwrapWait();
        SlokedDocumentSetClient documentClient(
            std::move(secondaryServer.GetServer()
                          .Connect({"/document/manager"})
                          .UnwrapWait()));
        documentClient
            .Open(inputPath.ToString(), mainConfig.Find("/encoding").AsString(),
                  mainConfig.Find("/newline").AsString(), "default")
            .UnwrapWait();

        // Screen layout
        screenClient.Handle.NewMultiplexer("/");
        auto mainWindow =
            screenClient.Multiplexer
                .NewWindow(
                    "/", TextPosition{0, 0},
                    TextPosition{
                        screenServer.GetScreen().GetSize().GetScreenSize().line,
                        screenServer.GetScreen()
                            .GetSize()
                            .GetScreenSize()
                            .column})
                .UnwrapWait();
        screenSizeClient.Listen([&](const auto &size) {
            mainEditor.GetThreadedExecutor().Enqueue([&, size] {
                if (screenServer.IsRunning() && mainWindow.has_value()) {
                    screenClient.Multiplexer
                        .ResizeWindow(mainWindow.value(), size)
                        .UnwrapWait();
                }
            });
        });
        screenClient.Handle.NewSplitter(mainWindow.value(),
                                        Splitter::Direction::Vertical);
        screenClient.Splitter
            .NewWindow(mainWindow.value(), Splitter::Constraints(1.0f))
            .UnwrapWait();
        auto tabber =
            screenClient.Splitter
                .NewWindow(mainWindow.value(), Splitter::Constraints(0.0f, 1))
                .UnwrapWait();
        screenClient.Handle.NewTabber("/0/0");
        auto tab1 = screenClient.Tabber.NewWindow("/0/0").UnwrapWait();
        screenClient.Handle.NewTextEditor(
            tab1.value(), documentClient.GetId().UnwrapWait().value(),
            "default");

        SlokedTextPaneClient paneClient(
            std::move(secondaryServer.GetServer()
                          .Connect({"/screen/component/text/pane"})
                          .UnwrapWait()));
        paneClient.Connect("/0/1", false, {}).UnwrapWait();
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
            std::move(secondaryServer.GetServer()
                          .Connect({"/screen/component/input/notify"})
                          .UnwrapWait()),
            Encoding::Get("system"));
        screenInput
            .Listen(
                "/",
                [&](auto &evt) {
                    mainEditor.GetExecutor().Enqueue([&, evt] {
                        renderStatus();
                        if (evt.value.index() != 0 &&
                            std::get<1>(evt.value) ==
                                SlokedControlKey::Escape) {
                            mainEditor.GetThreadedExecutor().Enqueue([&] {
                                logger.Debug() << "Saving document";
                                documentClient.Save(outputPath.ToString())
                                    .Notify([&terminate](const auto &) {
                                        terminate.Notify();
                                    });
                            });
                        }
                    });
                },
                true)
            .Wait();

        // Scripting engine startup
        std::unique_ptr<SlokedScriptEngine> scriptEngine;
        if (baseInterface.HasScripting() && mainConfig.Has("/script/init")) {
            scriptEngine = baseInterface.NewScriptEngine(
                manager, mainEditor.GetScheduler(), mainEditor.GetExecutor(),
                mainConfig.Find("/script/path").AsString());
            if (scriptEngine) {
                closeables.Attach(*scriptEngine);
                scriptEngine->Start(mainConfig.Find("/script/init").AsString());
            }
        }
        terminate.WaitAll();
        closeables.Close();
        return EXIT_SUCCESS;
    }

    extern "C" SlokedCorePlugin *SlokedGetCorePlugin() {
        return new SlokedFrontendDefaultCorePlugin();
    }
}  // namespace sloked