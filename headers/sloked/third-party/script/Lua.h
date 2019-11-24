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
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <map>
#include <lua.hpp>

namespace sloked {

    class SlokedLuaEngine;

    class LuaValueHandle {
     public:
        LuaValueHandle(SlokedLuaEngine &);
        ~LuaValueHandle();
        void Load();

     private:
        SlokedLuaEngine &engine;
        int ref;
    };

    class SlokedLuaEngine : public SlokedScriptEngine {
     public:
        SlokedLuaEngine();
        ~SlokedLuaEngine();
        void Start(const std::string &) final;
        void Stop() final;
        void BindServer(const std::string &, KgrNamedServer &) final;
        void Defer(std::shared_ptr<LuaValueHandle>);

        friend class LuaValueHandle;
    
     private:
        void Run(const std::string &);
        void InitializeGlobals();
        int SaveValue();
        void RestoreValue(int);
        void DropValue(int);

        lua_State *state;
        std::atomic<bool> work;
        std::thread workerThread;
        std::map<std::string, std::reference_wrapper<KgrNamedServer>> servers;
        SlokedSemaphore activity;
        std::mutex mtx;
        std::vector<int> refToRemove;
        std::vector<std::shared_ptr<LuaValueHandle>> callbacks;
    };
}

#endif