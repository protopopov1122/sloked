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

#include "sloked/editor/Application.h"

#include <iostream>

#include "sloked/services/DocumentSet.h"
#include "sloked/services/Screen.h"
#include "sloked/services/ScreenInput.h"
#include "sloked/services/Shutdown.h"
#include "sloked/services/ScreenSize.h"
#include "sloked/services/TextPane.h"
#include "sloked/text/fragment/Iterator.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/text/fragment/Updater.h"

namespace sloked {

    class SlokedFrontendDefaultApplication : public SlokedApplication {
     public:
        int Start(int, const char **, const SlokedBaseInterface &,
                  SlokedSharedContainerEnvironment &,
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
                this->encoding.GetCodepoint(currentLine, this->current.column)->start);
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

    int SlokedFrontendDefaultApplication::Start(
        int argc, const char **argv, const SlokedBaseInterface &baseInterface,
        SlokedSharedContainerEnvironment &sharedState,
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
                                KgrDictionary{{"options", "--load-application"},
                                              {"type", "string"},
                                              {"map", "/application"}}});
        cli.Parse(argc - 1, argv + 1);
        SlokedConfiguration mainConfig{
            cli.Export(), baseInterface.GetConfigurationLoader().Load("main"),
            DefaultConfiguration};
        if (cli.ArgCount() == 0) {
            std::cout << "Format: " << argv[0]
                      << " source -o destination [options]" << std::endl;
            return EXIT_FAILURE;
        }

        KgrDictionary primaryEditorConfig{
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
                                                  .AsInt()}}}}}}}};

        if (manager.HasCrypto()) {
            primaryEditorConfig["server"]
                .AsDictionary()["slave"]
                .AsDictionary()
                .Put("authorize", "user1");
            primaryEditorConfig.Put(
                "crypto",
                KgrDictionary{
                    {"salt", "salt"},
                    {"defaultKey",
                     KgrDictionary{
                         {"password", "password"},
                     }},
                    {"authentication",
                     KgrDictionary{
                         {"slave",
                          KgrDictionary{
                              {"users", KgrArray{KgrDictionary{
                                            {"id", "user1"},
                                            {"password", "password1"}}}}}}}}});
        }

        auto &primaryEditor =
            manager.Spawn("primary", primaryEditorConfig);
        auto &primaryServer = primaryEditor.GetServer();

        // Editor initialization
        SlokedTextPaneClient paneClient(
            std::move(primaryServer.GetServer()
                          .Connect({"/screen/component/text/pane"})
                          .UnwrapWait()));
        paneClient.Connect("/0/1", false, {}).UnwrapWait();
        auto &render = paneClient.GetRender();

        // Startup
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

        // Scripting engine startup
        std::unique_ptr<SlokedScriptEngine> scriptEngine;
        if (baseInterface.HasScripting() && mainConfig.Has("/script/init")) {
            scriptEngine = baseInterface.NewScriptEngine(
                manager, sharedState, mainConfig.Find("/script"));
            if (scriptEngine) {
                closeables.Attach(*scriptEngine);
                scriptEngine->Start();
                scriptEngine->Load(mainConfig.Find("/script/init").AsString())
                    .UnwrapWait();
            }
        }
        manager.GetTotalShutdown().WaitForShutdown();
        closeables.Close();
        return EXIT_SUCCESS;
    }

    extern "C" SlokedApplication *SlokedMakeApplication() {
        return new SlokedFrontendDefaultApplication();
    }
}  // namespace sloked