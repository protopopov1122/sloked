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

#include "sloked/third-party/script/lua/Sched.h"
#include "sloked/third-party/script/lua/Common.h"

namespace sloked {

    struct SchedTaskHandle {
        SchedTaskHandle(std::shared_ptr<SlokedSchedulerThread::TimerTask> task)
            : task(std::move(task)) {}

        std::shared_ptr<SlokedSchedulerThread::TimerTask> task;
    };

    static int SlokedSchedTask_GC(lua_State *state) {
        SchedTaskHandle *taskHandle = reinterpret_cast<SchedTaskHandle *>(luaL_checkudata(state, 1, "sloked.sched"));
        if (taskHandle != nullptr) {
            taskHandle->~SchedTaskHandle();
        }
        return 0;
    }

    static int SlokedSchedTask_Call(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 1) {
            return luaL_error(state, "sloked.sched.task(): Expected 0 arguments");
        }
        try {
            SchedTaskHandle *taskHandle = reinterpret_cast<SchedTaskHandle *>(luaL_checkudata(state, 1, "sloked.sched"));
            if (taskHandle != nullptr) {
                if (taskHandle->task) {
                    taskHandle->task->Cancel();
                    taskHandle->task = nullptr;
                }
                return 0;
            } else {
                return luaL_error(state, "sloked.sched.task(): Expected sloked.sched");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedSchedTaskToLua(lua_State *state, std::shared_ptr<SlokedSchedulerThread::TimerTask> task) {
        SchedTaskHandle *taskHandle = reinterpret_cast<SchedTaskHandle *>(lua_newuserdata(state, sizeof(SchedTaskHandle)));
        new(taskHandle) SchedTaskHandle(task);
        if (luaL_newmetatable(state, "sloked.sched.task")) {
            lua_pushcfunction(state, SlokedSchedTask_GC);
            lua_setfield(state, -2, "__gc");
            lua_pushcfunction(state, SlokedSchedTask_Call);
            lua_setfield(state, -2, "__call");
        }
        lua_setmetatable(state, -2);
        return 1;
    }

    struct SchedHandle {
        SchedHandle(SlokedSchedulerThread &sched, SlokedEventLoop &eventLoop)
            : sched(sched), eventLoop(eventLoop) {}

        SlokedSchedulerThread &sched;
        SlokedEventLoop &eventLoop;
    };

    static int SlokedSched_GC(lua_State *state) {
        SchedHandle *schedHandle = reinterpret_cast<SchedHandle *>(luaL_checkudata(state, 1, "sloked.sched"));
        if (schedHandle != nullptr) {
            schedHandle->~SchedHandle();
        }
        return 0;
    }

    static int SlokedSched_Defer(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 2) {
            return luaL_error(state, "sloked.sched.defer: Expected 2 arguments");
        }
        try {
            SchedHandle *sched = reinterpret_cast<SchedHandle *>(luaL_checkudata(state, 1, "sloked.sched"));
            if (sched == nullptr) {
                return luaL_error(state, "sloked.sched.defer: Expected sloked.sched");
            }
            if (lua_isfunction(state, 2)) {
                SlokedEventLoop &eventLoop = sched->eventLoop;
                lua_pushvalue(state, 2);
                auto callback = LuaCallback(state, eventLoop);
                sched->sched.Defer([&eventLoop, callback = std::move(callback)] {
                    eventLoop.Attach(callback);
                });
                return 0;
            } else {
                return luaL_error(state, "sloked.sched.defer: Expected function");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedSched_SetTimeout(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 3) {
            return luaL_error(state, "sloked.sched.setTimeout: Expected 3 arguments");
        }
        try {
            SchedHandle *sched = reinterpret_cast<SchedHandle *>(luaL_checkudata(state, 1, "sloked.sched"));
            if (sched == nullptr) {
                return luaL_error(state, "sloked.sched.defer: Expected sloked.sched");
            }
            if (lua_isfunction(state, 2) && lua_isinteger(state, 3)) {
                SlokedEventLoop &eventLoop = sched->eventLoop;
                lua_pushvalue(state, 2);
                auto callback = LuaCallback(state, eventLoop);
                int millis = lua_tointeger(state, 3);
                auto task = sched->sched.Sleep(std::chrono::milliseconds(millis), [&eventLoop, callback = std::move(callback)] {
                    eventLoop.Attach(callback);
                });
                return SlokedSchedTaskToLua(state, std::move(task));
            } else {
                return luaL_error(state, "sloked.sched.setTimeout: Expected function and integer");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    static int SlokedSched_SetInterval(lua_State *state) {
        int top = lua_gettop(state);
        if (top != 3) {
            return luaL_error(state, "sloked.sched.setInterval: Expected 3 arguments");
        }
        try {
            SchedHandle *sched = reinterpret_cast<SchedHandle *>(luaL_checkudata(state, 1, "sloked.sched"));
            if (sched == nullptr) {
                return luaL_error(state, "sloked.sched.defer: Expected sloked.sched");
            }
            if (lua_isfunction(state, 2) && lua_isinteger(state, 3)) {
                SlokedEventLoop &eventLoop = sched->eventLoop;
                lua_pushvalue(state, 2);
                auto callback = LuaCallback(state, eventLoop);
                int millis = lua_tointeger(state, 3);
                auto task = sched->sched.Interval(std::chrono::milliseconds(millis), [&eventLoop, callback = std::move(callback)] {
                    eventLoop.Attach(callback);
                });
                return SlokedSchedTaskToLua(state, std::move(task));
            } else {
                return luaL_error(state, "sloked.sched.setTimeout: Expected function and integer");
            }
        } catch (const SlokedError &err) {
            DropLuaStack(state, top);
            return luaL_error(state, err.what());
        }
    }

    int SlokedSchedToLua(SlokedSchedulerThread &sched, SlokedEventLoop &eventLoop, lua_State *state) {
        SchedHandle *schedHandle = reinterpret_cast<SchedHandle *>(lua_newuserdata(state, sizeof(SchedHandle)));
        new(schedHandle) SchedHandle(sched, eventLoop);
        if (luaL_newmetatable(state, "sloked.sched")) {
            lua_pushcfunction(state, SlokedSched_GC);
            lua_setfield(state, -2, "__gc");
            lua_newtable(state);
            lua_pushcfunction(state, SlokedSched_Defer);
            lua_setfield(state, -2, "defer");
            lua_pushcfunction(state, SlokedSched_SetTimeout);
            lua_setfield(state, -2, "setTimeout");
            lua_pushcfunction(state, SlokedSched_SetInterval);
            lua_setfield(state, -2, "setInterval");
            lua_setfield(state, -2, "__index");
        }
        lua_setmetatable(state, -2);
        return 1;
    }
}