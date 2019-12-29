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

#include "sloked/optional/script/lua/Logger.h"
#include "sloked/optional/script/lua/Common.h"

namespace sloked {

    static int SlokedLogger_Call(lua_State *state) {
        int top = lua_gettop(state);
        if (top < 1) {
            return luaL_error(state, "sloked.logger(): Expected at least 1 argument");
        }
        try {
            SlokedLogger **logger = reinterpret_cast<SlokedLogger **>(luaL_checkudata(state, 1, "sloked.logger"));
            if (logger == nullptr) {
                return luaL_error(state, "sloked.logger(): Expected sloked.logger");
            }
            auto entry = (**logger) << "";
            for (int i = 2; i <= top; i++) {
                entry << lua_tostring(state, i);
            }
            return 0;
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedLogger_To(lua_State *state) {
        int top = lua_gettop(state);
        if (top < 2) {
            return luaL_error(state, "sloked.logger:to(): Expected at least 2 arguments");
        }
        try {
            SlokedLogger **logger = reinterpret_cast<SlokedLogger **>(luaL_checkudata(state, 1, "sloked.logger"));
            int level = lua_tointeger(state, 2);
            if (logger == nullptr) {
                return luaL_error(state, "sloked.logger:to(): Expected sloked.logger");
            }
            auto entry = (*logger)->To(level);
            for (int i = 3; i <= top; i++) {
                entry << lua_tostring(state, i);
            }
            return 0;
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedLogger_Debug(lua_State *state) {
        int top = lua_gettop(state);
        if (top < 2) {
            return luaL_error(state, "sloked.logger:debug(): Expected at least 1 argument");
        }
        try {
            SlokedLogger **logger = reinterpret_cast<SlokedLogger **>(luaL_checkudata(state, 1, "sloked.logger"));
            if (logger == nullptr) {
                return luaL_error(state, "sloked.logger:debug(): Expected sloked.logger");
            }
            auto entry = (*logger)->Debug();
            for (int i = 2; i <= top; i++) {
                entry << lua_tostring(state, i);
            }
            return 0;
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }
    static int SlokedLogger_Info(lua_State *state) {
        int top = lua_gettop(state);
        if (top < 2) {
            return luaL_error(state, "sloked.logger:info(): Expected at least 1 argument");
        }
        try {
            SlokedLogger **logger = reinterpret_cast<SlokedLogger **>(luaL_checkudata(state, 1, "sloked.logger"));
            if (logger == nullptr) {
                return luaL_error(state, "sloked.logger:info(): Expected sloked.logger");
            }
            auto entry = (*logger)->Info();
            for (int i = 2; i <= top; i++) {
                entry << lua_tostring(state, i);
            }
            return 0;
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }
    static int SlokedLogger_Warning(lua_State *state) {
        int top = lua_gettop(state);
        if (top < 2) {
            return luaL_error(state, "sloked.logger:warning(): Expected at least 1 argument");
        }
        try {
            SlokedLogger **logger = reinterpret_cast<SlokedLogger **>(luaL_checkudata(state, 1, "sloked.logger"));
            if (logger == nullptr) {
                return luaL_error(state, "sloked.logger:warning(): Expected sloked.logger");
            }
            auto entry = (*logger)->Warning();
            for (int i = 2; i <= top; i++) {
                entry << lua_tostring(state, i);
            }
            return 0;
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }
    static int SlokedLogger_Error(lua_State *state) {
        int top = lua_gettop(state);
        if (top < 2) {
            return luaL_error(state, "sloked.logger:error(): Expected at least 1 argument");
        }
        try {
            SlokedLogger **logger = reinterpret_cast<SlokedLogger **>(luaL_checkudata(state, 1, "sloked.logger"));
            if (logger == nullptr) {
                return luaL_error(state, "sloked.logger:error(): Expected sloked.logger");
            }
            auto entry = (*logger)->Error();
            for (int i = 2; i <= top; i++) {
                entry << lua_tostring(state, i);
            }
            return 0;
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }
    static int SlokedLogger_Critical(lua_State *state) {
        int top = lua_gettop(state);
        if (top < 2) {
            return luaL_error(state, "sloked.logger:critical(): Expected at least 1 argument");
        }
        try {
            SlokedLogger **logger = reinterpret_cast<SlokedLogger **>(luaL_checkudata(state, 1, "sloked.logger"));
            if (logger == nullptr) {
                return luaL_error(state, "sloked.logger:to(): Expected sloked.logger");
            }
            auto entry = (*logger)->Critical();
            for (int i = 2; i <= top; i++) {
                entry << lua_tostring(state, i);
            }
            return 0;
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedLoggerLevels(lua_State *state) {
        lua_newtable(state);
        lua_newtable(state);
        lua_newtable(state);
        lua_pushinteger(state, static_cast<int>(SlokedLogLevel::Debug));
        lua_setfield(state, -2, "debug");
        lua_pushinteger(state, static_cast<int>(SlokedLogLevel::Info));
        lua_setfield(state, -2, "info");
        lua_pushinteger(state, static_cast<int>(SlokedLogLevel::Warning));
        lua_setfield(state, -2, "warning");
        lua_pushinteger(state, static_cast<int>(SlokedLogLevel::Error));
        lua_setfield(state, -2, "error");
        lua_pushinteger(state, static_cast<int>(SlokedLogLevel::Critical));
        lua_setfield(state, -2, "critical");
        lua_setfield(state, -2, "__index");
        lua_setmetatable(state, -2);
        return 1;
    }

    int SlokedLoggerToLua(SlokedLogger &loggerHandle, lua_State *state) {
        SlokedLogger **logger = reinterpret_cast<SlokedLogger **>(lua_newuserdata(state, sizeof(SlokedLogger *)));
        *logger = std::addressof(loggerHandle);
        if (luaL_newmetatable(state, "sloked.logger")) {
            lua_pushcfunction(state, SlokedLogger_Call);
            lua_setfield(state, -2, "__call");
            lua_newtable(state);
            lua_pushcfunction(state, SlokedLogger_To);
            lua_setfield(state, -2, "to");
            lua_pushcfunction(state, SlokedLogger_Debug);
            lua_setfield(state, -2, "debug");
            lua_pushcfunction(state, SlokedLogger_Info);
            lua_setfield(state, -2, "info");
            lua_pushcfunction(state, SlokedLogger_Warning);
            lua_setfield(state, -2, "warning");
            lua_pushcfunction(state, SlokedLogger_Error);
            lua_setfield(state, -2, "error");
            lua_pushcfunction(state, SlokedLogger_Critical);
            lua_setfield(state, -2, "critical");
            SlokedLoggerLevels(state);
            lua_setfield(state, -2, "level");
            lua_setfield(state, -2, "__index");
        }
        lua_setmetatable(state, -2);
        return 1;
    }
}