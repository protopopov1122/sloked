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

#ifndef SLOKED_THIRD_PARTY_LUA_COMMON_H_
#define SLOKED_THIRD_PARTY_LUA_COMMON_H_

#include <lua.hpp>

#include "sloked/kgr/Value.h"
#include "sloked/sched/EventLoop.h"

namespace sloked {

    void DropLuaStack(lua_State *, int);

    class LuaValueHandle {
     public:
        LuaValueHandle(lua_State *, SlokedEventLoop &);
        ~LuaValueHandle();
        void Load();

     private:
        lua_State *state;
        int ref;
        SlokedEventLoop &eventLoop;
    };

    std::function<void()> LuaCallback(lua_State *, SlokedEventLoop &);

    void KgrValueToLua(lua_State *, const KgrValue &);
    KgrValue LuaToKgrValue(lua_State *);
}  // namespace sloked

#endif