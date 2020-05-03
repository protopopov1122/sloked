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

#include "sloked/script/lua/Editor.h"

#include "sloked/script/lua/Server.h"

namespace sloked {

    struct SlokedAppHandle {
        SlokedAppHandle(SlokedEditorContainer &app) : app(app) {}

        SlokedEditorContainer &app;
    };

    static int SlokedApp_GC(lua_State *state) {
        SlokedAppHandle *appHandle =
            reinterpret_cast<SlokedAppHandle *>(lua_touserdata(state, 1));
        if (appHandle != nullptr) {
            appHandle->~SlokedAppHandle();
        }
        return 0;
    }

    int SlokedEditorToLua(SlokedEventLoop &eventLoop, lua_State *state,
                          SlokedEditorContainer &app) {
        SlokedAppHandle *appHandle = reinterpret_cast<SlokedAppHandle *>(
            lua_newuserdata(state, sizeof(SlokedAppHandle)));
        new (appHandle) SlokedAppHandle(app);
        if (luaL_newmetatable(state, "sloked.editor")) {
            lua_pushcfunction(state, SlokedApp_GC);
            lua_setfield(state, -2, "__gc");
            lua_newtable(state);
            SlokedServerToLua(eventLoop, state, app.GetServer().GetServer());
            lua_setfield(state, -2, "server");
            lua_setfield(state, -2, "__index");
        }
        lua_setmetatable(state, -2);
        return 1;
    }
}  // namespace sloked