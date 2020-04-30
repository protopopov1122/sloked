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
#include "sloked/kgr/Path.h"
#include "sloked/script/lua/Common.h"
#include "sloked/script/lua/Editor.h"
#include "sloked/script/lua/Logger.h"
#include "sloked/script/lua/Pipe.h"
#include "sloked/script/lua/Sched.h"

namespace sloked {

    SlokedLuaEngine::SlokedLuaEngine(SlokedEditorInstanceContainer &apps,
                                     SlokedScheduler &sched,
                                     SlokedExecutor &executor,
                                     const KgrValue &params)
        : state{nullptr}, work{false}, apps(apps), sched(sched),
          executor(executor), params{params}, logger(SlokedLoggerTag) {
        this->eventLoop.Notify([this] { this->activity.Notify(); });
    }

    SlokedLuaEngine::~SlokedLuaEngine() {
        this->Close();
    }

    void SlokedLuaEngine::Start() {
        if (!this->work.exchange(true)) {
            this->workerThread = std::thread([this] { this->Run(); });
        }
    }

    TaskResult<void> SlokedLuaEngine::Load(const std::string &script) {
        if (this->work.load()) {
            TaskResultSupplier<void> supplier;
            this->eventLoop.Attach([this, script, supplier] {
                if (luaL_dofile(this->state, script.c_str())) {
                    supplier.SetError(std::make_exception_ptr(
                        std::string{luaL_tolstring(this->state, -1, nullptr)}));
                    lua_pop(this->state, 1);
                } else {
                    supplier.SetResult();
                }
            });
            return supplier.Result();
        } else {
            return TaskResult<void>::Cancel();
        }
    }

    TaskResult<KgrValue> SlokedLuaEngine::Invoke(const std::string &method,
                                                 const KgrValue &params) {
        if (this->work.load()) {
            TaskResultSupplier<KgrValue> supplier;
            this->eventLoop.Attach([this, method, params, supplier] {
                lua_getglobal(this->state, method.c_str());
                KgrValueToLua(this->state, params);
                if (lua_pcall(this->state, 1, 1, 0)) {
                    supplier.SetError(std::make_exception_ptr(
                        std::string{luaL_tolstring(this->state, -1, nullptr)}));
                    lua_pop(this->state, 1);
                } else {
                    supplier.SetResult(LuaToKgrValue(this->state));
                }
            });
            return supplier.Result();
        } else {
            return TaskResult<KgrValue>::Cancel();
        }
    }

    TaskResult<KgrValue> SlokedLuaEngine::Eval(const std::string &script) {
        if (this->work.load()) {
            TaskResultSupplier<KgrValue> supplier;
            this->eventLoop.Attach([this, script, supplier] {
                if (luaL_loadstring(this->state, script.c_str()) ||
                    lua_pcall(this->state, 0, 1, 0)) {
                    supplier.SetError(std::make_exception_ptr(
                        std::string{luaL_tolstring(this->state, -1, nullptr)}));
                    lua_pop(this->state, 1);
                } else {
                    supplier.SetResult(LuaToKgrValue(this->state));
                }
            });
            return supplier.Result();
        } else {
            return TaskResult<KgrValue>::Cancel();
        }
    }

    void SlokedLuaEngine::Close() {
        if (this->work.exchange(false)) {
            this->activity.Notify();
            this->workerThread.join();
        }
    }

    void SlokedLuaEngine::Run() {
        this->state = luaL_newstate();
        this->InitializeGlobals();
        this->InitializePath();
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
        this->executor.Close();
        this->sched.Close();
        lua_close(this->state);
        this->state = nullptr;
    }

    void SlokedLuaEngine::InitializePath() {
        auto path = KgrPath::Traverse(this->params, {"/path"});
        if (path.has_value()) {
            lua_getglobal(state, "package");
            lua_getfield(state, -1, "path");
            lua_pushstring(state, ";");
            lua_pushstring(state, path.value().AsString().c_str());
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
        SlokedSchedToLua(this->sched, this->eventLoop, this->executor,
                         this->state);
        lua_setfield(this->state, -2, "sched");
        SlokedLoggerToLua(this->logger, this->state);
        lua_setfield(this->state, -2, "logger");
        lua_setglobal(this->state, "sloked");
    }
}  // namespace sloked