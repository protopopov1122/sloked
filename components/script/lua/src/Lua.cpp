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

#include "sloked/script/lua/Lua.h"

#include <iostream>

#include "sloked/core/Error.h"
#include "sloked/script/lua/Common.h"
#include "sloked/script/lua/Editor.h"
#include "sloked/script/lua/Logger.h"
#include "sloked/script/lua/Pipe.h"
#include "sloked/script/lua/Sched.h"

namespace sloked {

    SlokedLuaEngine::SlokedLuaEngine(SlokedEditorInstanceContainer &apps,
                                     SlokedSchedulerThread &sched,
                                     const std::string &path)
        : state{nullptr}, work{false}, apps(apps), sched(sched), path{path},
          logger(SlokedLoggerTag) {
        this->eventLoop.Notify([this] { this->activity.Notify(); });
    }

    SlokedLuaEngine::~SlokedLuaEngine() {
        this->Close();
    }

    void SlokedLuaEngine::Start(const std::string &script) {
        if (!this->work.exchange(true)) {
            this->workerThread =
                std::thread([this, script] { this->Run(script); });
        }
    }

    void SlokedLuaEngine::Close() {
        if (this->work.exchange(false)) {
            this->activity.Notify();
            this->workerThread.join();
        }
    }

    void SlokedLuaEngine::Run(const std::string &script) {
        this->state = luaL_newstate();
        this->InitializeGlobals();
        this->InitializePath();
        if (luaL_dofile(this->state, script.c_str())) {
            logger.To(SlokedLogLevel::Error)
                << luaL_tolstring(state, -1, nullptr);
        } else {
            try {
                while (this->work.load()) {
                    this->activity.WaitAll();
                    if (this->work.load() && this->eventLoop.HasPending()) {
                        this->eventLoop.Run();
                    }
                    this->sched.CollectGarbage();
                }
            } catch (const SlokedError &err) {
                logger.To(SlokedLogLevel::Error) << err.what();
            }
        }
        this->sched.DropAll();
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

    static int SlokedAppContainer_Get(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 2) {
            return luaL_error(state, "sloked.editors[]: Expected 2 argument");
        }
        try {
            std::string key(lua_tostring(state, 2));
            SlokedEditorInstanceContainer &apps =
                *reinterpret_cast<SlokedEditorInstanceContainer *>(
                    lua_touserdata(state, lua_upvalueindex(1)));
            SlokedEventLoop &eventLoop = *reinterpret_cast<SlokedEventLoop *>(
                lua_touserdata(state, lua_upvalueindex(2)));
            auto &app = apps.Get(key);
            SlokedEditorToLua(eventLoop, state, app);
            lua_pushvalue(state, 2);
            lua_pushvalue(state, -2);
            lua_rawset(state, 1);
            return 1;
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    void SlokedLuaEngine::InitializeApps() {
        if (luaL_newmetatable(this->state, "sloked.editors")) {
            lua_pushlightuserdata(this->state, std::addressof(this->apps));
            lua_pushlightuserdata(this->state, std::addressof(this->eventLoop));
            lua_pushcclosure(this->state, SlokedAppContainer_Get, 2);
            lua_setfield(this->state, -2, "__index");
        }
        lua_setmetatable(this->state, -2);
    }

    void SlokedLuaEngine::InitializeGlobals() {
        luaL_openlibs(this->state);
        lua_newtable(this->state);
        lua_newtable(this->state);
        this->InitializeApps();
        lua_setfield(this->state, -2, "editors");
        SlokedSchedToLua(this->sched, this->eventLoop, this->state);
        lua_setfield(this->state, -2, "sched");
        SlokedLoggerToLua(this->logger, this->state);
        lua_setfield(this->state, -2, "logger");
        lua_setglobal(this->state, "sloked");
    }
}  // namespace sloked