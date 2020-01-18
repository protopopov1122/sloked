/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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
#include "sloked/core/Closeable.h"
#include "sloked/core/Monitor.h"
#include "sloked/core/Logger.h"
#include "sloked/core/CLI.h"
#include "sloked/core/Semaphore.h"
#include "sloked/core/awaitable/Poll.h"
#include "sloked/screen/terminal/Compat.h"
#include "sloked/screen/terminal/multiplexer/TerminalBuffer.h"
#include "sloked/namespace/Virtual.h"
#include "sloked/namespace/Resolve.h"
#include "sloked/namespace/Mount.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/kgr/local/Pipe.h"
#include "sloked/kgr/local/Server.h"
#include "sloked/kgr/local/NamedServer.h"
#include "sloked/kgr/ctx-manager/RunnableContextManager.h"
#include "sloked/services/TextRender.h"
#include "sloked/services/Cursor.h"
#include "sloked/services/DocumentNotify.h"
#include "sloked/services/DocumentSet.h"
#include "sloked/services/Namespace.h"
#include "sloked/services/Screen.h"
#include "sloked/services/ScreenInput.h"
#include "sloked/services/ScreenSize.h"
#include "sloked/services/Search.h"
#include "sloked/services/TextPane.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"
#include "sloked/editor/terminal/ScreenProvider.h"
#include "sloked/net/Compat.h"
#include "sloked/kgr/net/MasterServer.h"
#include "sloked/kgr/net/SlaveServer.h"
#include "sloked/editor/Configuration.h"
#include "sloked/kgr/Path.h"
#include "sloked/sched/Scheduler.h"
#include "sloked/kgr/net/Config.h"
#include "sloked/script/Compat.h"
#include "sloked/crypto/Compat.h"
#include "sloked/security/Master.h"
#include "sloked/security/Slave.h"
#include "sloked/net/CryptoSocket.h"
#include "sloked/core/Failure.h"
#include "sloked/core/awaitable/Compat.h"
#include "sloked/namespace/Compat.h"
#include "sloked/screen/terminal/TerminalResize.h"
#include "sloked/screen/terminal/TerminalSize.h"
#include "sloked/editor/EditorApp.h"
#include "sloked/facade/Services.h"
#include "sloked/editor/Startup.h"
#include "sloked/namespace/Root.h"
#include "sloked/namespace/Empty.h"
#include <chrono>

using namespace sloked;

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

class SlokedDemoSystemNamespace : public SlokedRootNamespace {
 public:
    SlokedDemoSystemNamespace()
        : root(std::make_unique<SlokedEmptyNamespace>()),
          mounter(SlokedNamespaceCompat::NewRootFilesystem(), root),
          resolver(SlokedNamespaceCompat::GetWorkDir(), SlokedNamespaceCompat::GetHomeDir()) {}

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

class SlokedDemoScreenBasis : public SlokedScreenProvider {
 public:
    SlokedDemoScreenBasis(const SlokedCharWidth &charWidth)
        : terminal(*SlokedTerminalCompat::GetSystemTerminal()),
          console(terminal, Encoding::Get("system"), charWidth),
          provider(console, Encoding::Get("system"), charWidth, terminal) {}
        
    void Render(std::function<void(SlokedScreenComponent &)> fn) final {
        this->provider.Render(std::move(fn));
    }

    std::vector<SlokedKeyboardInput> ReceiveInput(std::chrono::system_clock::duration timeout) final {
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
    BufferedTerminal console;
    SlokedTerminalScreenProvider<SlokedTerminalResizeListener> provider;
};

class SlokedDemoScreenFactory : public SlokedScreenProviderFactory {
 public:
    std::unique_ptr<SlokedScreenProvider> Make(const SlokedUri &, const SlokedCharWidth &charWidth) final {
        return std::make_unique<SlokedDemoScreenBasis>(charWidth);
    }
};

static const KgrDictionary DefaultConfiguration {
    { "encoding", "system" },
    { "newline", "system" },
    {
        "network", KgrDictionary {
            { "port", 1234 },
        }
    },
    {
        "script", KgrDictionary {
            { "init", "" },
            { "path", "" }
        }
    }
};

int main(int argc, const char **argv) {
    // Initialize globals
    SlokedFailure::SetupHandler();
    SlokedLocale::Setup();
    SlokedLoggingManager::Global.SetSink(SlokedLogLevel::Debug, SlokedLoggingSink::TextFile("./sloked.log", SlokedLoggingSink::TabularFormat(10, 30, 30)));
    SlokedLogger logger(SlokedLoggerTag);
    const Encoding &terminalEncoding = Encoding::Get("system");
    SlokedCloseablePool closeables;
    SlokedDemoRootNamespaceFactory nsFactory;
    SlokedDefaultTextTaggerRegistry<int> baseTaggers;
    baseTaggers.Bind("default", std::make_unique<TestFragmentFactory>());
    SlokedEditorStartup::Parameters startupPrms(logger, nsFactory);
    startupPrms.SetTaggers(baseTaggers);
    if constexpr (SlokedCryptoCompat::IsSupported()) {
        startupPrms.SetCrypto(SlokedCryptoCompat::GetCrypto());
    }
    startupPrms.SetEditors([] {
        return std::make_unique<SlokedEditorApp>(SlokedIOPollCompat::NewPoll(), SlokedNetCompat::GetNetwork());
    });
    if constexpr (!SlokedTerminalCompat::HasSystemTerminal()) {
        throw SlokedError("Demo: system terminal is required");
    }
    SlokedDemoScreenFactory screenFactory;
    startupPrms.SetScreenProviders(screenFactory);
    SlokedEditorStartup startup(std::move(startupPrms));
    closeables.Attach(startup);

    // Configuration
    SlokedXdgConfiguration mainConfig("main", DefaultConfiguration);
    SlokedCLI cli;
    cli.Define("--encoding", mainConfig.Find("/encoding").AsString());
    cli.Define("--newline", mainConfig.Find("/newline").AsString());
    cli.Define("-o,--output", cli.Option<std::string>());
    cli.Define("--net-port", mainConfig.Find("/network/port").AsInt());
    cli.Define("--script", mainConfig.Find("/script/init").AsString());
    cli.Define("--script-path", mainConfig.Find("/script/path").AsString());
    cli.Parse(argc, argv);
    if (cli.Size() == 0) {
        std::cout << "Format: " << argv[0] << " source -o destination [options]" << std::endl;
        return EXIT_FAILURE;
    }

    // Main editor
    KgrDictionary mainEditorConfig {
        {
            "crypto", KgrDictionary {
                { "masterPassword", "password" },
                { "salt", "salt" },
                {
                    "authentication", KgrDictionary {
                        {
                            "master", KgrDictionary {
                                { "masterPassword", "password" },
                                { "salt", "salt" },
                                {
                                    "users", KgrArray {
                                        KgrDictionary {
                                            { "id", "user1" },
                                            {
                                                "restrictAccess", KgrDictionary {
                                                    { "whitelist", true },
                                                    { "content", KgrArray {"document::", "namespace::", "screen::"} }
                                                }
                                            },
                                            {
                                                "restrictModification", KgrDictionary {
                                                    { "whitelist", true },
                                                    { "content", KgrArray {"screen::"} }
                                                }
                                            }
                                        }
                                    }
                                },
                                {
                                    "defaultUser", KgrDictionary {
                                        {
                                            "restrictAccess", KgrDictionary {
                                                { "whitelist", true },
                                                { "content", KgrArray {} }
                                            }
                                        },
                                        {
                                            "restrictModification", KgrDictionary {
                                                { "whitelist", true },
                                                { "content", KgrArray {} }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        },
        {
            "server", KgrDictionary {
                {
                    "netServer", KgrDictionary {
                        { "host", "localhost" },
                        { "port", cli["net-port"].As<int64_t>() }
                    }
                },
                {
                    "restrictAccess", KgrDictionary {
                        { "whitelist", true },
                        { "content", KgrArray {"document::", "namespace::", "screen::"} }
                    }
                },
                {
                    "restrictModification", KgrDictionary {
                        { "whitelist", true },
                        { "content", KgrArray {"document::", "namespace::", "screen::"} }
                    }
                },
                {
                    "services", KgrDictionary {
                        { "root", "file:///" },
                        {
                            "endpoints", KgrArray {
                                "document::render",
                                "document::cursor",
                                "document::manager",
                                "document::notify",
                                "document::search",
                                "namespace::root"
                            }
                        }
                    }
                }
            }
        }
    };
    auto &mainEditor = startup.Spawn("main", mainEditorConfig);

    KgrDictionary secondaryEditorConfig {
        {
            "server", KgrDictionary {
                {
                    "slave", KgrDictionary {
                        {
                            "address", KgrDictionary {
                                { "host", "localhost" },
                                { "port", cli["net-port"].As<int64_t>() }
                            }
                        }
                    }
                },
                { "screen", "terminal:///system" }
            }
        }
    };

    if constexpr (SlokedCryptoCompat::IsSupported()) {
        secondaryEditorConfig["server"].AsDictionary()["slave"].AsDictionary().Put("authorize", "user1");
        secondaryEditorConfig.Put("crypto", KgrDictionary {
            { "masterPassword", "password" },
            { "salt", "salt" },
            {
                "authentication", KgrDictionary {
                    {
                        "slave", KgrDictionary {
                            {
                                "users", KgrArray {
                                    KgrDictionary {
                                        { "id", "user1" },
                                        { "credentials", mainEditor.GetCrypto().GetCredentialMaster().GetAccountByName("user1").lock()->GetCredentials() }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        });
    }

    auto &secondaryEditor = startup.Spawn("secondary", secondaryEditorConfig);
    auto &secondaryServer = secondaryEditor.GetServer();

    auto &serviceProvider = mainEditor.GetServiceProvider();
    SlokedPath inputPath = serviceProvider.GetNamespace().GetResolver().Resolve(SlokedPath{cli.At(0)});
    SlokedPath outputPath = serviceProvider.GetNamespace().GetResolver().Resolve(SlokedPath{cli["output"].As<std::string>()});

    // Screen
    auto &screenServer = secondaryEditor.GetScreen();


    // Editor initialization
    auto isScreenLocked = [&screenServer] {
        return screenServer.GetScreen().GetScreen().IsHolder();
    };
    SlokedScreenClient screenClient(secondaryServer.GetServer().Connect("screen::manager"), isScreenLocked);
    SlokedScreenSizeNotificationClient screenSizeClient(secondaryServer.GetServer().Connect("screen::size.notify"));
    SlokedDocumentSetClient documentClient(secondaryServer.GetServer().Connect("document::manager"));
    documentClient.Open(inputPath.ToString(), cli["encoding"].As<std::string>(), cli["newline"].As<std::string>());

    // Screen layout
    screenClient.Handle.NewMultiplexer("/");
    auto mainWindow = screenClient.Multiplexer.NewWindow("/", TextPosition{0, 0}, TextPosition{screenServer.GetScreen().GetSize().GetSize().line, screenServer.GetScreen().GetSize().GetSize().column});
    screenSizeClient.Listen([&](const auto &size) {
        if (screenServer.IsRunning() && mainWindow.has_value()) {
            mainEditor.GetScheduler().Defer([&, size] {
                screenClient.Multiplexer.ResizeWindow(mainWindow.value(), size);
            });
        }
    });
    screenClient.Handle.NewSplitter(mainWindow.value(), Splitter::Direction::Vertical);
    screenClient.Splitter.NewWindow(mainWindow.value(), Splitter::Constraints(1.0f));
    auto tabber = screenClient.Splitter.NewWindow(mainWindow.value(), Splitter::Constraints(0.0f, 1));
    screenClient.Handle.NewTabber("/0/0");
    auto tab1 = screenClient.Tabber.NewWindow("/0/0");
    screenClient.Handle.NewTextEditor(tab1.value(), documentClient.GetId().value(), "default");

    SlokedTextPaneClient paneClient(secondaryServer.GetServer().Connect("screen::component::text.pane"), isScreenLocked);
    paneClient.Connect("/0/1", false, {});
    auto &render = paneClient.GetRender();

    // Startup
    SlokedSemaphore terminate;
    int i = 0;
    auto renderStatus = [&] {
        render.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
        render.ClearScreen();
        render.SetPosition(0, 0);
        render.Write(std::to_string(i++));
        render.Flush();
    };
    renderStatus();
    SlokedScreenInputNotificationClient screenInput(secondaryServer.GetServer().Connect("screen::component::input.notify"), terminalEncoding, isScreenLocked);
    screenInput.Listen("/", [&](auto &evt) {
        mainEditor.GetScheduler().Defer([&, evt] {
            renderStatus();
            if (evt.value.index() != 0 && std::get<1>(evt.value) == SlokedControlKey::Escape) {
                logger.Debug() << "Saving document";
                documentClient.Save(outputPath.ToString());
                terminate.Notify();
            }
        });
    }, true);

    // Scripting engine startup
    std::unique_ptr<SlokedScriptEngine> scriptEngine;
    if constexpr (SlokedScriptCompat::IsSupported()) {
        scriptEngine = SlokedScriptCompat::GetEngine(startup, mainEditor.GetScheduler(), cli["script-path"].As<std::string>());
        closeables.Attach(*scriptEngine);
        if (cli.Has("script") && !cli["script"].As<std::string>().empty()) {
            scriptEngine->Start(cli["script"].As<std::string>());
        }
    }
    terminate.WaitAll();
    closeables.Close();
    return EXIT_SUCCESS;
}