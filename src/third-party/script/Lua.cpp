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

#include "sloked/third-party/script/Lua.h"
#include "sloked/core/Error.h"
#include "sloked/core/Logger.h"

namespace sloked {
    static SlokedLogger logger(SlokedLoggerTag);

    SlokedLuaEngine::SlokedLuaEngine()
        : state{nullptr}, work{false} {}

    SlokedLuaEngine::~SlokedLuaEngine() {
        this->Stop();
    }

    void SlokedLuaEngine::Start(const std::string &script) {
        if (!this->work.exchange(true)) {
            this->workerThread = std::thread([this, script] {
                this->Run(script);
            });
        }
    }

    void SlokedLuaEngine::Stop() {
        if (this->work.exchange(false)) {
            this->activity.Notify();
            this->workerThread.join();
        }
    }

    void SlokedLuaEngine::BindServer(const std::string &serverId, KgrNamedServer &srv) {
        if (!this->work.load()) {
            this->servers.emplace(serverId, std::ref(srv));
        } else {
            throw SlokedError("LuaEngine: Can't bind server to running engine");
        }
    }

    void SlokedLuaEngine::Run(const std::string &script) {
        this->state = luaL_newstate();
        this->InitializeGlobals();
        if (luaL_dofile(this->state, script.c_str())) {
            logger.To(SlokedLogLevel::Error) << lua_tostring(state, -1);
        } else {
            while (this->work.load()) {
                this->activity.WaitAll();
                if (!this->work.load()) {
                    break;
                }
                std::unique_lock lock(this->mtx);
                for (auto idx : this->refToRemove) {
                    luaL_unref(this->state, LUA_REGISTRYINDEX, idx);
                }
                this->refToRemove.clear();
                while (!this->callbacks.empty()) {
                    auto ref = std::move(this->callbacks.front());
                    this->callbacks.erase(this->callbacks.begin());
                    lock.unlock();
                    ref->Load();
                    lua_pcall(this->state, 0, 0, 0);
                    lock.lock();
                }
            }
        }
        lua_close(this->state);
        this->state = nullptr;
    }

    void SlokedLuaEngine::Defer(std::shared_ptr<LuaValueHandle> handle) {
        std::unique_lock lock(this->mtx);
        this->callbacks.push_back(std::move(handle));
        this->activity.Notify();
    }

    struct SlokedPipeHandle {
        SlokedPipeHandle(std::unique_ptr<KgrPipe> pipe)
            : pipe(std::move(pipe)) {}
        std::unique_ptr<KgrPipe> pipe;
    };

    static int SlokedPipe_GC(lua_State *state) {
        SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
        pipe->~SlokedPipeHandle();
        return 0;
    }

    static int SlokedPipe_IsOpen(lua_State *state) {
        if (lua_gettop(state) != 1) {
            return luaL_error(state, "sloked.pipe,isOpen: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
            lua_pushboolean(state, pipe->pipe->GetStatus() == KgrPipe::Status::Open);
            return 1;
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_Empty(lua_State *state) {
        if (lua_gettop(state) != 1) {
            return luaL_error(state, "sloked.pipe.empty: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
            lua_pushboolean(state, pipe->pipe->Empty());
            return 1;
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_Count(lua_State *state) {
        if (lua_gettop(state) != 1) {
            return luaL_error(state, "sloked.pipe.count: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
            lua_pushinteger(state, pipe->pipe->Count());
            return 1;
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_Close(lua_State *state) {
        if (lua_gettop(state) != 1) {
            return luaL_error(state, "sloked.pipe.close: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
            pipe->pipe->Close();
            return 0;
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    static void KgrValueToLua(lua_State *state, const KgrValue &value) {
        switch (value.GetType()) {
            case KgrValueType::Null:
                lua_pushnil(state);
                break;

            case KgrValueType::Integer:
                lua_pushinteger(state, value.AsInt());
                break;

            case KgrValueType::Number:
                lua_pushnumber(state, value.AsNumber());
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

    static int SlokedPipe_Read(lua_State *state) {
        if (lua_gettop(state) != 1) {
            return luaL_error(state, "sloked.pipe.read: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
            auto msg = pipe->pipe->Read();
            KgrValueToLua(state, msg);
            return 1;
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_ReadOptional(lua_State *state) {
        if (lua_gettop(state) != 1) {
            return luaL_error(state, "sloked.pipe.readOptional: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
            auto msg = pipe->pipe->ReadOptional();
            if (msg.has_value()) {
                KgrValueToLua(state, msg.value());
                return 1;
            } else {
                return 0;
            }
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_ReadWait(lua_State *state) {
        if (lua_gettop(state) != 1) {
            return luaL_error(state, "sloked.pipe.readWait: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
            auto msg = pipe->pipe->ReadWait();
            KgrValueToLua(state, msg);
            return 1;
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    static KgrValue LuaToKgrValue(lua_State *state) {
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
                return lua_toboolean(state, -1);

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
                    pairs.push_back(std::make_pair(std::move(key), std::move(value)));
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
                            array.emplace(array.begin() + (idx - 1), std::move(kv.second));
                        } else {
                            throw SlokedError("LuaToKgrValue: Invalid key type");
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

    static int SlokedPipe_Wait(lua_State *state) {
        if (lua_gettop(state) < 1) {
            return luaL_error(state, "sloked.pipe.wait: Expected 1 or 2 arguments");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
            int count = 1;
            if (lua_isinteger(state, 2)) {
                count = lua_tointeger(state, 2);
            }
            lua_pushboolean(state, pipe->pipe->Wait(count));
            return 1;
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_Drop(lua_State *state) {
        if (lua_gettop(state) < 1) {
            return luaL_error(state, "sloked.pipe.drop: Expected 1 or 2 arguments");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
            int count = 1;
            if (lua_isinteger(state, 2)) {
                count = lua_tointeger(state, 2);
            }
            pipe->pipe->Drop(count);
            return 0;
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_DropAll(lua_State *state) {
        if (lua_gettop(state) != 1) {
            return luaL_error(state, "sloked.pipe.dropAll: Expected 1 argument");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
            pipe->pipe->DropAll();
            return 0;
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_Write(lua_State *state) {
        if (lua_gettop(state) != 2) {
            return luaL_error(state, "sloked.pipe.write: Expected 2 arguments");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
            lua_pushvalue(state, 2);    
            auto msg = LuaToKgrValue(state);
            lua_pop(state, 1);
            pipe->pipe->Write(std::move(msg));
            return 0;
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    static int SlokedPipe_WriteNX(lua_State *state) {
        if (lua_gettop(state) != 2) {
            return luaL_error(state, "sloked.pipe.writeNX: Expected 2 arguments");
        }
        try {
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
            lua_pushvalue(state, 2);    
            auto msg = LuaToKgrValue(state);
            lua_pop(state, 1);
            lua_pushboolean(state, pipe->pipe->WriteNX(std::move(msg)));
            return 1;
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    LuaValueHandle::LuaValueHandle(SlokedLuaEngine &engine)
        : engine(engine) {
        this->ref = engine.SaveValue();
    }

    LuaValueHandle::~LuaValueHandle() {
        engine.DropValue(this->ref);
    }

    void LuaValueHandle::Load() {
        engine.RestoreValue(this->ref);
    }

    static int SlokedPipe_Listen(lua_State *state) {
        if (lua_gettop(state) != 2) {
            return luaL_error(state, "sloked.pipe.listen: Expected 2 arguments");
        }
        try {
            SlokedLuaEngine &engine = *reinterpret_cast<SlokedLuaEngine *>(lua_touserdata(state, lua_upvalueindex(1)));
            SlokedPipeHandle *pipe = reinterpret_cast<SlokedPipeHandle *>(lua_touserdata(state, 1));
            if (lua_isfunction(state, 2)) {
                lua_pushvalue(state, 2);    
                auto handle = std::make_shared<LuaValueHandle>(engine);
                pipe->pipe->SetMessageListener([&engine, handle = std::move(handle)] {
                    engine.Defer(handle);
                });
            } else {
                pipe->pipe->SetMessageListener(nullptr);
            }
            return 0;
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    static int BuildPipe(SlokedLuaEngine &engine, lua_State *state, std::unique_ptr<KgrPipe> pipe) {
        SlokedPipeHandle *pipeHandle = reinterpret_cast<SlokedPipeHandle *>(lua_newuserdata(state, sizeof(SlokedPipeHandle)));
        new(pipeHandle) SlokedPipeHandle(std::move(pipe));
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
            lua_setfield(state, -2, "readOptional");
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
            lua_pushcfunction(state, SlokedPipe_WriteNX);
            lua_setfield(state, -2, "writeNX");
            lua_pushlightuserdata(state, std::addressof(engine));
            lua_pushcclosure(state, SlokedPipe_Listen, 1);
            lua_setfield(state, -2, "listen");
            lua_setfield(state, -2, "__index");
        }
        lua_setmetatable(state, -2);
        return 1;
    }

    static int SlokedServer_Connect(lua_State *state) {
        if (lua_gettop(state) != 2) {
            return luaL_error(state, "sloked.server.connect: Expected 2 arguments");
        }
        try {
            SlokedLuaEngine &engine = *reinterpret_cast<SlokedLuaEngine *>(lua_touserdata(state, lua_upvalueindex(1)));
            KgrNamedServer &srv = *reinterpret_cast<KgrNamedServer *>(lua_touserdata(state, 1));
            std::string serviceName{lua_tostring(state, 2)};
            if (srv.Registered(serviceName)) {
                return BuildPipe(engine, state, srv.Connect(serviceName));
            } else {
                return 0;
            }
        } catch (const SlokedError &err) {
            return luaL_error(state, err.what());
        }
    }

    static void BindServerToLua(SlokedLuaEngine &engine, lua_State *state, const std::string &serverId, KgrNamedServer &srv) {
        lua_pushlightuserdata(state, std::addressof(srv));
        if (luaL_newmetatable(state, "sloked.server")) {
            lua_newtable(state);
            lua_pushlightuserdata(state, std::addressof(engine));
            lua_pushcclosure(state, SlokedServer_Connect, 1);
            lua_setfield(state, -2, "connect");
            lua_setfield(state, -2, "__index");
        }
        lua_setmetatable(state, -2);
        lua_setfield(state, -2, serverId.c_str());
    }

    void SlokedLuaEngine::InitializeGlobals() {
        luaL_openlibs(this->state);
        lua_newtable(this->state);
        lua_newtable(this->state);
        for (auto &kv : this->servers) {
            BindServerToLua(*this, this->state, kv.first, kv.second.get());
        }
        lua_setfield(this->state, -2, "servers");
        lua_setglobal(this->state, "sloked");
    }

    int SlokedLuaEngine::SaveValue() {
        return luaL_ref(this->state, LUA_REGISTRYINDEX);
    }

    void SlokedLuaEngine::RestoreValue(int idx) {
        lua_geti(state, LUA_REGISTRYINDEX, idx);
    }

    void SlokedLuaEngine::DropValue(int idx) {
        std::unique_lock lock(this->mtx);
        this->refToRemove.push_back(idx);
        this->activity.Notify();
    }
}