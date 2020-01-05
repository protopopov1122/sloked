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
#include "sloked/screen/terminal/posix/PosixTerminal.h"
#include "sloked/screen/terminal/multiplexer/TerminalBuffer.h"
#include "sloked/namespace/Virtual.h"
#include "sloked/namespace/Resolve.h"
#include "sloked/namespace/posix/Environment.h"
#include "sloked/namespace/posix/Filesystem.h"
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
#include "sloked/net/PosixSocket.h"
#include "sloked/kgr/net/MasterServer.h"
#include "sloked/kgr/net/SlaveServer.h"
#include "sloked/editor/Configuration.h"
#include "sloked/kgr/Path.h"
#include "sloked/sched/Scheduler.h"
#include "sloked/kgr/net/Config.h"
#include "sloked/script/lua/Lua.h"
#include "sloked/crypto/botan/Botan.h"
#include "sloked/editor/EditorCore.h"
#include "sloked/security/Master.h"
#include "sloked/security/Slave.h"
#include "sloked/net/CryptoSocket.h"
#include "sloked/core/Failure.h"
#include "sloked/screen/terminal/TerminalSize.h"
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
    // Core initialization
    SlokedFailure::SetupHandler();
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

    SlokedLoggingManager::Global.SetSink(SlokedLogLevel::Debug, SlokedLoggingSink::TextFile("./sloked.log", SlokedLoggingSink::TabularFormat(10, 30, 30)));
    SlokedLogger logger(SlokedLoggerTag);

    // Cryptography & security
    SlokedBotanCrypto crypto;
    auto key = crypto.DeriveKey("password", "salt");
    SlokedCredentialMaster authMaster(crypto, *key);
    SlokedCredentialSlave authSlave(crypto);
    SlokedAuthenticatorFactory authFactory(crypto, authMaster, "salt");
    SlokedAuthenticatorFactory authSlaveFactory(crypto, authSlave, "salt");
    auto user = authMaster.New("user1").lock();
    user->SetAccessRestrictions(SlokedNamedWhitelist::Make({"document::", "namespace::", "screen::"}));
    user->SetModificationRestrictions(SlokedNamedWhitelist::Make({"screen::"}));
    authSlave.New(user->GetName(), user->GetCredentials());
    authMaster.GetDefaultAccount().lock()->SetAccessRestrictions(SlokedNamedWhitelist::Make({}));
    authMaster.GetDefaultAccount().lock()->SetModificationRestrictions(SlokedNamedWhitelist::Make({}));

    // Editor initialization
    SlokedCloseablePool closeables;
    SlokedDefaultSchedulerThread sched;
    closeables.Attach(sched);
    sched.Start();
    SlokedPosixAwaitablePoll socketPoll;
    SlokedDefaultIOPollThread socketPoller(socketPoll);
    closeables.Attach(socketPoller);
    socketPoller.Start(KgrNetConfig::RequestTimeout);
    SlokedPosixSocketFactory socketFactory;
    // SlokedCryptoSocketFactory socketFactory(rawSocketFactory, crypto, *key);
    SlokedDefaultVirtualNamespace root(std::make_unique<SlokedFilesystemNamespace>(std::make_unique<SlokedPosixFilesystemAdapter>("/")));
    SlokedDefaultNamespaceMounter mounter(std::make_unique<SlokedPosixFilesystemAdapter>("/"), root);
    SlokedCharWidth charWidth;

    SlokedEditorMasterCore editor(logger, socketPoller, root, mounter, charWidth);
    closeables.Attach(editor);
    editor.Start();
    editor.SpawnNetServer(socketFactory, "localhost", cli["net-port"].As<int>(), &authMaster, &authFactory);
    editor.GetTaggers().Bind("default", std::make_unique<TestFragmentFactory>());
    editor.GetRestrictions().SetAccessRestrictions(SlokedNamedWhitelist::Make({"document::", "namespace::", "screen::"}));
    editor.GetRestrictions().SetModificationRestrictions(SlokedNamedWhitelist::Make({"document::", "namespace::", "screen::"}));

    // Proxy initialization
    SlokedEditorSlaveCore slaveEditor(socketFactory.Connect("localhost", cli["net-port"].As<int>()), logger, socketPoller, &authSlaveFactory);
    closeables.Attach(slaveEditor);
    slaveEditor.Start();
    slaveEditor.Authorize("user1");
    auto &slaveServer = slaveEditor.GetServer();

    user->RevokeCredentials();
    authSlave.GetAccountByName(user->GetName()).lock()->ChangeCredentials(user->GetCredentials());

    // Screen initialization
    SlokedLocale::Setup();
    const Encoding &terminalEncoding = Encoding::Get("system");

    PosixTerminal terminal;
    BufferedTerminal console(terminal, terminalEncoding, charWidth);
    SlokedTerminalSize terminalSize(console);
    SlokedTerminalScreenProvider screen(console, terminalEncoding, charWidth, terminal);

    SlokedScreenServer screenServer(slaveServer, screen, terminalSize, editor.GetContextManager());
    closeables.Attach(screenServer);
    screenServer.Start(KgrNetConfig::RequestTimeout);

    // Editor configuration
    SlokedPathResolver resolver(SlokedPosixNamespaceEnvironment::WorkDir(), SlokedPosixNamespaceEnvironment::HomeDir());
    SlokedPath inputPath = resolver.Resolve(SlokedPath{cli.At(0)});
    SlokedPath outputPath = resolver.Resolve(SlokedPath{cli["output"].As<std::string>()});

    auto isScreenLocked = [&screen] {
        return screen.GetScreen().IsHolder();
    };
    SlokedScreenClient screenClient(slaveServer.Connect("screen::manager"), isScreenLocked);
    SlokedScreenSizeNotificationClient screenSizeClient(slaveServer.Connect("screen::size.notify"));
    SlokedDocumentSetClient documentClient(slaveServer.Connect("document::manager"));
    documentClient.Open(inputPath.ToString(), cli["encoding"].As<std::string>(), cli["newline"].As<std::string>());

    // Screen layout
    screenClient.Handle.NewMultiplexer("/");
    auto mainWindow = screenClient.Multiplexer.NewWindow("/", TextPosition{0, 0}, TextPosition{console.GetHeight(), console.GetWidth()});
    screenSizeClient.Listen([&](const auto &size) {
        if (screenServer.IsRunning() && mainWindow.has_value()) {
            sched.Defer([&, size] {
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

    SlokedTextPaneClient paneClient(slaveServer.Connect("screen::component::text.pane"), isScreenLocked);
    paneClient.Connect("/0/1", false, {});
    auto &render = paneClient.GetRender();

    // Startup
    SlokedSemaphore work;
    int i = 0;
    auto renderStatus = [&] {
        render.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
        render.ClearScreen();
        render.SetPosition(0, 0);
        render.Write(std::to_string(i++));
        render.Flush();
    };
    renderStatus();
    SlokedScreenInputNotificationClient screenInput(slaveServer.Connect("screen::component::input.notify"), terminalEncoding, isScreenLocked);
    screenInput.Listen("/", [&](auto &evt) {
        editor.GetScheduler().Defer([&, evt] {
            renderStatus();
            if (evt.value.index() != 0 && std::get<1>(evt.value) == SlokedControlKey::Escape) {
                logger.Debug() << "Saving document";
                documentClient.Save(outputPath.ToString());
                work.Notify();
            }
        });
    }, true);

    // Scripting engine startup
    SlokedLuaEngine luaEngine(editor.GetScheduler(), cli["script-path"].As<std::string>());
    closeables.Attach(luaEngine);
    luaEngine.BindServer("main", slaveServer);
    if (cli.Has("script") && !cli["script"].As<std::string>().empty()) {
        luaEngine.Start(cli["script"].As<std::string>());
    }

    // Wait until editor finishes
    work.Wait();
    closeables.Close();
    return EXIT_SUCCESS;
}