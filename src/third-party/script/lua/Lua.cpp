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

#include "sloked/third-party/script/lua/Lua.h"
#include "sloked/third-party/script/lua/Common.h"
#include "sloked/third-party/script/lua/Pipe.h"
#include "sloked/third-party/script/lua/Sched.h"
#include "sloked/third-party/script/lua/Server.h"
#include "sloked/core/Error.h"
#include "sloked/core/Logger.h"
#include <iostream>

namespace sloked {
    static SlokedLogger logger(SlokedLoggerTag);

    SlokedLuaEngine::SlokedLuaEngine(const std::string &path)
        : state{nullptr}, work{false}, path{path} {
        this->eventLoop.Notify([this] {
            this->activity.Notify();
        });
    }

    SlokedLuaEngine::~SlokedLuaEngine() {
        this->Stop();
    }

    void SlokedLuaEngine::Start(const std::string &script) {
        if (!this->work.exchange(true)) {
            this->workerThread = std::thread([this, script] {
                this->Run(script);
            });
        }
    }

    void SlokedLuaEngine::Stop() {
        if (this->work.exchange(false)) {
            this->activity.Notify();
            this->workerThread.join();
        }
    }

    void SlokedLuaEngine::BindServer(const std::string &serverId, KgrNamedServer &srv) {
        if (!this->work.load()) {
            this->servers.emplace(serverId, std::ref(srv));
        } else {
            throw SlokedError("LuaEngine: Can't bind server to running engine");
        }
    }

    void SlokedLuaEngine::Run(const std::string &script) {
        this->sched.Start();
        this->state = luaL_newstate();
        this->InitializeGlobals();
        this->InitializePath();
        if (luaL_dofile(this->state, script.c_str())) {
            logger.To(SlokedLogLevel::Error) << luaL_tolstring(state, -1, nullptr);
        } else {
            try {
                while (this->work.load()) {
                    this->activity.WaitAll();
                    if (this->work.load() && this->eventLoop.HasPending()) {
                            this->eventLoop.Run();
                    }
                }
            } catch (const SlokedError &err) {
                logger.To(SlokedLogLevel::Error) << err.what();
            }
        }
        this->sched.Stop();
        lua_close(this->state);
        this->state = nullptr;
    }

    void SlokedLuaEngine::InitializePath() {
        if (!this->path.empty()) {
            lua_getglobal(state, "package");
            lua_getfield(state, -1, "path");
            lua_pushstring(state, ";");
            lua_pushstring(state, this->path.c_str());
            lua_concat(state, 3);
            lua_setfield(state, -2, "path");
            lua_pop(state, 1);
        }
    }

    void SlokedLuaEngine::InitializeGlobals() {
        luaL_openlibs(this->state);
        lua_newtable(this->state);
        lua_newtable(this->state);
        for (auto &kv : this->servers) {
            SlokedServerToLua(this->eventLoop, this->state, kv.second.get());
            lua_setfield(state, -2, kv.first.c_str());
        }
        lua_setfield(this->state, -2, "servers");
        SlokedSchedToLua(this->sched, this->eventLoop, this->state);
        lua_setfield(this->state, -2, "sched");
        lua_setglobal(this->state, "sloked");
    }
}