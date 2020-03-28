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

#include "sloked/script/lua/Server.h"

#include "sloked/core/Error.h"
#include "sloked/script/lua/Common.h"
#include "sloked/script/lua/Pipe.h"

namespace sloked {

    static int SlokedServer_Connect(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 2) {
            return luaL_error(state,
                              "sloked.server.connect: Expected 2 arguments");
        }
        try {
            SlokedEventLoop &eventLoop = *reinterpret_cast<SlokedEventLoop *>(
                lua_touserdata(state, lua_upvalueindex(1)));
            KgrNamedServer **srvPtr = reinterpret_cast<KgrNamedServer **>(
                luaL_checkudata(state, 1, "sloked.server"));
            if (srvPtr == nullptr) {
                return luaL_error(
                    state, "sloked.server.connect: Expected sloked.server");
            }
            KgrNamedServer &srv = **srvPtr;
            std::string serviceName{lua_tostring(state, 2)};
            if (srv.Registered({serviceName})) {
                return KgrPipeToLua(eventLoop, state,
                                    srv.Connect({serviceName}));
            } else {
                return 0;
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    int SlokedServerToLua(SlokedEventLoop &eventLoop, lua_State *state,
                          KgrNamedServer &srv) {
        KgrNamedServer **srvPtr = reinterpret_cast<KgrNamedServer **>(
            lua_newuserdata(state, sizeof(KgrNamedServer *)));
        *srvPtr = std::addressof(srv);
        if (luaL_newmetatable(state, "sloked.server")) {
            lua_newtable(state);
            lua_pushlightuserdata(state, std::addressof(eventLoop));
            lua_pushcclosure(state, SlokedServer_Connect, 1);
            lua_setfield(state, -2, "connect");
            lua_setfield(state, -2, "__index");
        }
        lua_setmetatable(state, -2);
        return 1;
    }
}  // namespace sloked