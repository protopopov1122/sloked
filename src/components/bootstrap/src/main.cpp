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
#include "sloked/compat/net/Compat.h"
#include "sloked/core/CLI.h"
#include "sloked/core/Closeable.h"
#include "sloked/core/Failure.h"
#include "sloked/core/Locale.h"
#include "sloked/core/Logger.h"
#include "sloked/editor/Application.h"
#include "sloked/editor/EditorContainer.h"
#include "sloked/editor/Manager.h"

using namespace sloked;

std::pair<std::unique_ptr<SlokedApplication>,
          std::unique_ptr<SlokedDynamicLibrary>>
    LoadApplication(int argc, const char **argv,
                   const SlokedBaseInterface &baseInterface) {
    SlokedCLI cli;
    cli.Define("--load-application", cli.Option<std::string>());
    cli.Parse(argc - 1, argv + 1, true);
    if (cli.Has("load-application")) {
        auto libraryPath = cli["load-application"].As<std::string>();
        auto library = baseInterface.GetDynamicLibraryLoader().Load(
            libraryPath, SlokedDynamicLibrary::Binding::Lazy,
            SlokedDynamicLibrary::Scope::Local);
        void *pluginGetterRaw = library->Resolve("SlokedMakeApplication");
        SlokedApplicationFactory pluginGetter =
            reinterpret_cast<SlokedApplicationFactory>(pluginGetterRaw);
        return std::make_pair(std::unique_ptr<SlokedApplication>(pluginGetter()),
                              std::move(library));
    } else {
#ifdef SLOKED_BOOTSTRAP_HAS_DEFAULT_APPLICATION
        return std::make_pair(
            std::unique_ptr<SlokedApplication>(SlokedMakeApplication()), nullptr);
#else
        throw SlokedError("Bootstrap: No default core plugin defined");
#endif
    }
}

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
    SlokedSharedContainerEnvironment sharedEditorState(SlokedIOPollCompat::NewPoll());
    closeables.Attach(sharedEditorState);
    sharedEditorState.Start();

    startupPrms.SetEditors([&sharedEditorState] {
        return std::make_unique<SlokedEditorContainer>(
            sharedEditorState, SlokedNetCompat::GetNetwork());
    });
    if constexpr (SlokedCompressionCompat::IsSupported()) {
        startupPrms.SetComresssion(SlokedCompressionCompat::GetCompression());
    }
    SlokedBootstrapScreenFactory screenFactory;
    startupPrms.SetScreenProviders(screenFactory);
    auto startup =
        std::make_unique<SlokedEditorManager>(std::move(startupPrms));
    closeables.Attach(*startup);

    auto [application, applicationLibrary] =
        LoadApplication(argc, argv, BaseInterface);
    auto rc = application->Start(argc, argv, BaseInterface, sharedEditorState,
                                *startup);

    closeables.Close();
    application.reset();
    startup.reset();
    return rc;
}