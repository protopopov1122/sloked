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

#include "sloked/third-party/script/lua/Common.h"

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
}