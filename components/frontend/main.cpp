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
#include "sloked/frontend/CorePlugin.h"
#include "sloked/frontend/Graphics.h"
#include "sloked/frontend/Namespace.h"
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

class SlokedFrontendScriptEngineFactory : public SlokedScriptEngineFactory {
    std::unique_ptr<SlokedScriptEngine> Make(
        SlokedEditorInstanceContainer &container,
        SlokedSchedulerThread &scheduler, const std::string &path) final {
        return SlokedScriptCompat::GetEngine(container, scheduler, path);
    }
};

int main(int argc, const char **argv) {
    // Initialize globals
    SlokedFailure::SetupHandler();
    SlokedLocale::Setup();
    SlokedLogger logger(SlokedLoggerTag);
    SlokedCloseablePool closeables;
    SlokedFrontendRootNamespaceFactory nsFactory;
    SlokedEditorManager::Parameters startupPrms(
        logger, nsFactory, SlokedConfigurationLoaderCompat::GetLoader());
    if constexpr (SlokedCryptoCompat::IsSupported()) {
        startupPrms.SetCrypto(SlokedCryptoCompat::GetCrypto());
    }
    startupPrms.SetEditors([] {
        return std::make_unique<SlokedEditorInstance>(
            SlokedIOPollCompat::NewPoll(), SlokedNetCompat::GetNetwork());
    });
    if constexpr (SlokedCompressionCompat::IsSupported()) {
        startupPrms.SetComresssion(SlokedCompressionCompat::GetCompression());
    }
    SlokedFrontendScriptEngineFactory scripting;
    if constexpr (SlokedScriptCompat::IsSupported()) {
        startupPrms.SetScriptEngineFactory(scripting);
    }
    SlokedFrontendScreenFactory screenFactory;
    startupPrms.SetScreenProviders(screenFactory);
    SlokedEditorManager startup(std::move(startupPrms));
    closeables.Attach(startup);

    SlokedFrontendDefaultCorePlugin corePlugin;
    auto rc = corePlugin.Start(argc, argv, startup);

    closeables.Close();
    return rc;
}