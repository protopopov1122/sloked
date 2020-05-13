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
#include "sloked/services/ScreenSize.h"
#include "sloked/services/Shutdown.h"
#include "sloked/services/TextPane.h"
namespace sloked {

    class SlokedFrontendDefaultApplication : public SlokedApplication {
     public:
        int Start(int, const char **, const SlokedBaseInterface &,
                  SlokedSharedContainerEnvironment &,
                  SlokedEditorManager &) final;
    };

    static const KgrValue DefaultConfiguration =
        KgrDictionary{{"network", KgrDictionary{
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
        // Configuration
        SlokedCLI cli;
        cli.Initialize(KgrArray{KgrDictionary{{"options", "--net-port"},
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
        manager.Spawn("primary", primaryEditorConfig);

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