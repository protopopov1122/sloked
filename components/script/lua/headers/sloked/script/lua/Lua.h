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

#ifndef SLOKED_THIRD_PARTY_SCRIPT_LUA_H_
#define SLOKED_THIRD_PARTY_SCRIPT_LUA_H_

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include "sloked/core/Logger.h"
#include "sloked/core/Semaphore.h"
#include "sloked/editor/EditorInstance.h"
#include "sloked/sched/EventLoop.h"
#include "sloked/sched/ScopedExecutor.h"
#include "sloked/sched/ScopedScheduler.h"
#include "sloked/script/ScriptEngine.h"
#include "sloked/script/lua/Base.h"

namespace sloked {

    class SlokedLuaEngine : public SlokedScriptEngine {
     public:
        SlokedLuaEngine(SlokedEditorInstanceContainer &, SlokedScheduler &,
                        SlokedExecutor &, const KgrValue & = {});
        ~SlokedLuaEngine();
        void Start() final;
        TaskResult<void> Load(const std::string &) final;
        TaskResult<KgrValue> Invoke(const std::string &,
                                    const KgrValue &) final;
        TaskResult<KgrValue> Eval(const std::string &) final;
        void Close() final;

     private:
        void Run();
        void InitializePath();
        void InitializeApps();
        void InitializeGlobals();

        lua_State *state;
        std::atomic<bool> work;
        std::thread workerThread;
        SlokedEditorInstanceContainer &apps;
        SlokedSemaphore activity;
        SlokedDefaultEventLoop eventLoop;
        SlokedScopedScheduler sched;
        SlokedScopedExecutor executor;
        KgrValue params;
        SlokedLogger logger;
    };
}  // namespace sloked

#endif
