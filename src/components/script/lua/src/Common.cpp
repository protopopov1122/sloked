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

    std::function<void()> LuaCallback(lua_State *state,
                                      SlokedEventLoop &eventLoop) {
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

    void KgrValueToLua(lua_State *state, const KgrValue &value) {
        switch (value.GetType()) {
            case KgrValueType::Null:
                lua_pushnil(state);
                break;

            case KgrValueType::Integer:
                lua_pushinteger(state, value.AsInt());
                break;

            case KgrValueType::Float:
                lua_pushnumber(state, value.AsFloat());
                break;

            case KgrValueType::Boolean:
                lua_pushboolean(state, value.AsBoolean());
                break;

            case KgrValueType::String:
                lua_pushstring(state, value.AsString().c_str());
                break;

            case KgrValueType::Array: {
                auto &array = value.AsArray();
                lua_newtable(state);
                int index = 1;
                for (auto &el : array) {
                    KgrValueToLua(state, el);
                    lua_seti(state, -2, index++);
                }
            } break;

            case KgrValueType::Object: {
                auto &obj = value.AsDictionary();
                lua_newtable(state);
                for (auto &el : obj) {
                    lua_pushstring(state, el.first.c_str());
                    KgrValueToLua(state, el.second);
                    lua_settable(state, -3);
                }
            } break;
        }
    }

    KgrValue LuaToKgrValue(lua_State *state) {
        switch (lua_type(state, -1)) {
            case LUA_TNIL:
                return {};

            case LUA_TNUMBER:
                if (lua_isinteger(state, -1)) {
                    return static_cast<int64_t>(lua_tointeger(state, -1));
                } else {
                    return lua_tonumber(state, -1);
                }

            case LUA_TBOOLEAN:
                return static_cast<bool>(lua_toboolean(state, -1));

            case LUA_TSTRING:
                return lua_tostring(state, -1);

            case LUA_TTABLE: {
                bool array = true;
                std::vector<std::pair<KgrValue, KgrValue>> pairs;
                lua_pushnil(state);
                while (lua_next(state, -2) != 0) {
                    KgrValue value = LuaToKgrValue(state);
                    lua_pop(state, 1);
                    KgrValue key = LuaToKgrValue(state);
                    pairs.push_back(
                        std::make_pair(std::move(key), std::move(value)));
                    if (!key.Is(KgrValueType::Integer)) {
                        array = false;
                    } else if (!key.Is(KgrValueType::String)) {
                        throw SlokedError("LuaToKgrValue: Invalid key type");
                    }
                }
                if (array) {
                    std::vector<KgrValue> array;
                    array.insert(array.end(), pairs.size(), {});
                    for (auto &kv : pairs) {
                        std::size_t idx = kv.first.AsInt();
                        if (idx <= array.size() || idx > 0) {
                            array.emplace(array.begin() + (idx - 1),
                                          std::move(kv.second));
                        } else {
                            throw SlokedError(
                                "LuaToKgrValue: Invalid key type");
                        }
                    }
                    return KgrArray{std::move(array)};
                } else {
                    KgrDictionary dict;
                    for (auto &kv : pairs) {
                        dict.Put(kv.first.AsString(), std::move(kv.second));
                    }
                    return dict;
                }
            } break;
        }

        throw SlokedError("LuaToKgrValue: Unexpected value type");
    }
}  // namespace sloked