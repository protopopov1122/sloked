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

#include "sloked/script/lua/Pipe.h"

#include <iostream>

#include "sloked/core/Error.h"
#include "sloked/script/lua/Common.h"

namespace sloked {

    struct SlokedPipeHandle {
        SlokedPipeHandle(std::unique_ptr<KgrPipe> pipe)
            : pipe(std::move(pipe)) {}
        std::unique_ptr<KgrPipe> pipe;
    };

    static int SlokedPipe_GC(lua_State *state) {
        SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
            luaL_checkudata(state, 1, "sloked.pipe"));
        if (pipe != nullptr && pipe->pipe != nullptr) {
            pipe->pipe->SetMessageListener(nullptr);
            pipe->pipe->Close();
            pipe->~SlokedPipeHandle();
        }
        return 0;
    }

    static int SlokedPipe_IsOpen(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 1) {
            return luaL_error(state, "sloked.pipe.isOpen: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
                luaL_checkudata(state, 1, "sloked.pipe"));
            if (pipe != nullptr && pipe->pipe != nullptr) {
                lua_pushboolean(
                    state, pipe->pipe->GetStatus() == KgrPipe::Status::Open);
                return 1;
            } else {
                return luaL_error(state, "sloked.pipe.isOpen: Expected pipe");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_Empty(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 1) {
            return luaL_error(state, "sloked.pipe.empty: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
                luaL_checkudata(state, 1, "sloked.pipe"));
            if (pipe != nullptr && pipe->pipe != nullptr) {
                lua_pushboolean(state, pipe->pipe->Empty());
                return 1;
            } else {
                return luaL_error(state, "sloked.pipe.empty: Expected pipe");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_Count(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 1) {
            return luaL_error(state, "sloked.pipe.count: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
                luaL_checkudata(state, 1, "sloked.pipe"));
            if (pipe != nullptr && pipe->pipe != nullptr) {
                lua_pushinteger(state, pipe->pipe->Count());
                return 1;
            } else {
                return luaL_error(state, "sloked.pipe.count: Expected pipe");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_Close(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 1) {
            return luaL_error(state, "sloked.pipe.close: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
                luaL_checkudata(state, 1, "sloked.pipe"));
            if (pipe != nullptr && pipe->pipe != nullptr) {
                pipe->pipe->SetMessageListener(nullptr);
                pipe->pipe->Close();
                return 0;
            } else {
                return luaL_error(state, "sloked.pipe.close: Expected pipe");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_Read(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 1) {
            return luaL_error(state, "sloked.pipe.read: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
                luaL_checkudata(state, 1, "sloked.pipe"));
            if (pipe != nullptr && pipe->pipe != nullptr) {
                auto msg = pipe->pipe->Read();
                KgrValueToLua(state, msg);
                return 1;
            } else {
                return luaL_error(state, "sloked.pipe.read: Expected pipe");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_ReadOptional(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 1) {
            return luaL_error(state,
                              "sloked.pipe.tryRead: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
                luaL_checkudata(state, 1, "sloked.pipe"));
            if (pipe != nullptr && pipe->pipe != nullptr) {
                auto msg = pipe->pipe->ReadOptional();
                if (msg.has_value()) {
                    KgrValueToLua(state, msg.value());
                    return 1;
                } else {
                    return 0;
                }
            } else {
                return luaL_error(state, "sloked.pipe.tryRead: Expected pipe");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_ReadWait(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 1) {
            return luaL_error(state,
                              "sloked.pipe.readWait: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
                luaL_checkudata(state, 1, "sloked.pipe"));
            if (pipe != nullptr && pipe->pipe != nullptr) {
                auto msg = pipe->pipe->ReadWait();
                KgrValueToLua(state, msg);
                return 1;
            } else {
                return luaL_error(state, "sloked.pipe.readWait: Expected pipe");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_Wait(lua_State *state) {
        int top = lua_gettop(state);
        if (top < 1) {
            return luaL_error(state,
                              "sloked.pipe.wait: Expected 1 or 2 arguments");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
                luaL_checkudata(state, 1, "sloked.pipe"));
            if (pipe != nullptr && pipe->pipe != nullptr) {
                int count = 1;
                if (lua_isinteger(state, 2)) {
                    count = lua_tointeger(state, 2);
                }
                lua_pushboolean(state, pipe->pipe->Wait(count));
                return 1;
            } else {
                return luaL_error(state, "sloked.pipe.wait: Expected pipe");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_Drop(lua_State *state) {
        int top = lua_gettop(state);
        if (top < 1) {
            return luaL_error(state,
                              "sloked.pipe.drop: Expected 1 or 2 arguments");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
                luaL_checkudata(state, 1, "sloked.pipe"));
            if (pipe != nullptr && pipe->pipe != nullptr) {
                int count = 1;
                if (lua_isinteger(state, 2)) {
                    count = lua_tointeger(state, 2);
                }
                pipe->pipe->Drop(count);
                return 0;
            } else {
                return luaL_error(state, "sloked.pipe.drop: Expected pipe");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_DropAll(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 1) {
            return luaL_error(state,
                              "sloked.pipe.dropAll: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
                luaL_checkudata(state, 1, "sloked.pipe"));
            if (pipe != nullptr && pipe->pipe != nullptr) {
                pipe->pipe->DropAll();
                return 0;
            } else {
                return luaL_error(state, "sloked.pipe.dropAll: Expected pipe");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_Write(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 2) {
            return luaL_error(state, "sloked.pipe.write: Expected 2 arguments");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
                luaL_checkudata(state, 1, "sloked.pipe"));
            if (pipe != nullptr && pipe->pipe != nullptr) {
                lua_pushvalue(state, 2);
                auto msg = LuaToKgrValue(state);
                lua_pop(state, 1);
                pipe->pipe->Write(std::move(msg));
                return 0;
            } else {
                return luaL_error(state, "sloked.pipe.write: Expected pipe");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_SafeWrite(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 2) {
            return luaL_error(state,
                              "sloked.pipe.tryWrite: Expected 2 arguments");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
                luaL_checkudata(state, 1, "sloked.pipe"));
            if (pipe != nullptr && pipe->pipe != nullptr) {
                lua_pushvalue(state, 2);
                auto msg = LuaToKgrValue(state);
                lua_pop(state, 1);
                lua_pushboolean(state, pipe->pipe->SafeWrite(std::move(msg)));
                return 1;
            } else {
                return luaL_error(state, "sloked.pipe.tryWrite: Expected pipe");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_Listen(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 2) {
            return luaL_error(state,
                              "sloked.pipe.listen: Expected 2 arguments");
        }
        try {
            SlokedEventLoop &eventLoop = *reinterpret_cast<SlokedEventLoop *>(
                lua_touserdata(state, lua_upvalueindex(1)));
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(
                luaL_checkudata(state, 1, "sloked.pipe"));
            if (pipe != nullptr && pipe->pipe != nullptr) {
                if (lua_isfunction(state, 2)) {
                    lua_pushvalue(state, 2);
                    auto callback = LuaCallback(state, eventLoop);
                    pipe->pipe->SetMessageListener(
                        [&eventLoop, callback = std::move(callback)] {
                            eventLoop.Attach(callback);
                        });
                } else {
                    pipe->pipe->SetMessageListener(nullptr);
                }
                return 0;
            } else {
                return luaL_error(state, "sloked.pipe.listen: Expected pipe");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    int KgrPipeToLua(SlokedEventLoop &eventLoop, lua_State *state,
                     std::unique_ptr<KgrPipe> pipe) {
        SlokedPipeHandle *pipeHandle = reinterpret_cast<SlokedPipeHandle *>(
            lua_newuserdata(state, sizeof(SlokedPipeHandle)));
        new (pipeHandle) SlokedPipeHandle(std::move(pipe));
        if (luaL_newmetatable(state, "sloked.pipe")) {
            lua_pushcfunction(state, SlokedPipe_GC);
            lua_setfield(state, -2, "__gc");
            lua_newtable(state);
            lua_pushcfunction(state, SlokedPipe_IsOpen);
            lua_setfield(state, -2, "isOpen");
            lua_pushcfunction(state, SlokedPipe_Empty);
            lua_setfield(state, -2, "empty");
            lua_pushcfunction(state, SlokedPipe_Count);
            lua_setfield(state, -2, "count");
            lua_pushcfunction(state, SlokedPipe_Close);
            lua_setfield(state, -2, "close");
            lua_pushcfunction(state, SlokedPipe_Read);
            lua_setfield(state, -2, "read");
            lua_pushcfunction(state, SlokedPipe_ReadOptional);
            lua_setfield(state, -2, "tryRead");
            lua_pushcfunction(state, SlokedPipe_ReadWait);
            lua_setfield(state, -2, "readWait");
            lua_pushcfunction(state, SlokedPipe_Wait);
            lua_setfield(state, -2, "wait");
            lua_pushcfunction(state, SlokedPipe_Drop);
            lua_setfield(state, -2, "drop");
            lua_pushcfunction(state, SlokedPipe_DropAll);
            lua_setfield(state, -2, "dropAll");
            lua_pushcfunction(state, SlokedPipe_Write);
            lua_setfield(state, -2, "write");
            lua_pushcfunction(state, SlokedPipe_SafeWrite);
            lua_setfield(state, -2, "tryWrite");
            lua_pushlightuserdata(state, std::addressof(eventLoop));
            lua_pushcclosure(state, SlokedPipe_Listen, 1);
            lua_setfield(state, -2, "listen");
            lua_setfield(state, -2, "__index");
        }
        lua_setmetatable(state, -2);
        return 1;
    }
}  // namespace sloked