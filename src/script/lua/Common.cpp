/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#include "sloked/script/lua/Common.h"

namespace sloked {

    void DropLuaStack(lua_State *state, int top) {
        int newTop = lua_gettop(state);
        if (newTop > top) {
            lua_pop(state, newTop - top);
        }
    }

    LuaValueHandle::LuaValueHandle(lua_State *state, SlokedEventLoop &eventLoop)
        : state(state), eventLoop(eventLoop) {
        this->ref = luaL_ref(this->state, LUA_REGISTRYINDEX);
    }

    LuaValueHandle::~LuaValueHandle() {
        this->eventLoop.Attach([state = this->state, ref = this->ref] {
            luaL_unref(state, LUA_REGISTRYINDEX, ref);
            return false;
        });
    }

    void LuaValueHandle::Load() {
        lua_geti(state, LUA_REGISTRYINDEX, this->ref);
    }

    std::function<void()> LuaCallback(lua_State *state, SlokedEventLoop &eventLoop) {
        lua_geti(state, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
        auto mainThread = lua_tothread(state, -1);
        lua_pop(state, 1);
        lua_xmove(state, mainThread, 1);
        auto handle = std::make_shared<LuaValueHandle>(mainThread, eventLoop);
        return [state = mainThread, functionHandle = std::move(handle)] {
            functionHandle->Load();
            if (lua_pcall(state, 0, 0, 0) != 0) {
                const char *msg = luaL_tolstring(state, -1, nullptr);
                throw SlokedError(msg != nullptr ? msg : "");
            }
        };
    }
}