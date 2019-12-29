/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

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

#ifndef SLOKED_THIRD_PARTY_SCRIPT_LUA_H_
#define SLOKED_THIRD_PARTY_SCRIPT_LUA_H_

#include "sloked/script/ScriptEngine.h"
#include "sloked/core/Semaphore.h"
#include "sloked/sched/EventLoop.h"
#include "sloked/sched/Scheduler.h"
#include "sloked/core/Logger.h"
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <map>
#include <lua.hpp>

namespace sloked {

    class SlokedLuaEngine : public SlokedScriptEngine {
     public:
        SlokedLuaEngine(SlokedSchedulerThread &, const std::string & = "");
        ~SlokedLuaEngine();
        void Start(const std::string &) final;
        void Close() final;
        void BindServer(const std::string &, KgrNamedServer &) final;
    
     private:
        void Run(const std::string &);
        void InitializePath();
        void InitializeGlobals();

        lua_State *state;
        std::atomic<bool> work;
        std::thread workerThread;
        std::map<std::string, std::reference_wrapper<KgrNamedServer>> servers;
        SlokedSemaphore activity;
        SlokedDefaultEventLoop eventLoop;
        SlokedScheduledTaskPool sched;
        std::string path;
        SlokedLogger logger;
    };
}

#endif