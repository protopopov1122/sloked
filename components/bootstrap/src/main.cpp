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

#include "sloked/bootstrap/Graphics.h"
#include "sloked/bootstrap/Namespace.h"
#include "sloked/compat/Interface.h"
#include "sloked/compat/compression/Compat.h"
#include "sloked/compat/core/awaitable/Compat.h"
#include "sloked/compat/crypto/Compat.h"
#include "sloked/compat/editor/configuration/Compat.h"
#include "sloked/compat/namespace/Compat.h"
#include "sloked/compat/net/Compat.h"
#include "sloked/compat/screen/graphics/Compat.h"
#include "sloked/compat/screen/terminal/Compat.h"
#include "sloked/compat/screen/terminal/TerminalResize.h"
#include "sloked/compat/script/Compat.h"
#include "sloked/core/CLI.h"
#include "sloked/core/Closeable.h"
#include "sloked/core/Failure.h"
#include "sloked/core/Locale.h"
#include "sloked/core/Logger.h"
#include "sloked/core/Monitor.h"
#include "sloked/core/Semaphore.h"
#include "sloked/core/awaitable/Poll.h"
#include "sloked/editor/Configuration.h"
#include "sloked/editor/CorePlugin.h"
#include "sloked/editor/EditorInstance.h"
#include "sloked/editor/Manager.h"
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
#include "sloked/namespace/Empty.h"
#include "sloked/namespace/Mount.h"
#include "sloked/namespace/Resolve.h"
#include "sloked/namespace/Root.h"
#include "sloked/namespace/Virtual.h"
#include "sloked/net/CryptoSocket.h"
#include "sloked/sched/Scheduler.h"
#include "sloked/screen/terminal/TerminalSize.h"
#include "sloked/screen/terminal/multiplexer/TerminalBuffer.h"
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

int main(int argc, const char **argv) {
    // Initialize globals
    SlokedFailure::SetupHandler();
    SlokedLocale::Setup();
    SlokedLogger logger(SlokedLoggerTag);
    SlokedCloseablePool closeables;
    SlokedBootstrapRootNamespaceFactory nsFactory;
    const SlokedBaseInterface &BaseInterface =
        SlokedEditorCompat::GetBaseInterface();
    SlokedEditorManager::Parameters startupPrms(logger, nsFactory);
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
    SlokedBootstrapScreenFactory screenFactory;
    startupPrms.SetScreenProviders(screenFactory);
    SlokedEditorManager startup(std::move(startupPrms));
    closeables.Attach(startup);

    auto corePlugin = SlokedGetCorePlugin();
    auto rc = corePlugin->Start(argc, argv, BaseInterface, startup);

    closeables.Close();
    return rc;
}