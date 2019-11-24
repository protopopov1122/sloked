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

#ifndef SLOKED_SCHED_EVENTLOOP_H_
#define SLOKED_SCHED_EVENTLOOP_H_

#include "sloked/Base.h"
#include <functional>
#include <queue>
#include <memory>
#include <map>
#include <mutex>

namespace sloked {

    class SlokedAsyncTask {
     public:
        virtual ~SlokedAsyncTask() = default;
        virtual void Wait(std::function<void()>) = 0;
        virtual bool Run() = 0;
    };

    class SlokedDynamicAsyncTask : public SlokedAsyncTask {
        using Callback = std::function<std::function<bool()>(std::function<void()>)>;
     public:
        SlokedDynamicAsyncTask(Callback);
        void Wait(std::function<void()>) final;
        bool Run() final;

     private:
        Callback callback;
        std::function<bool()> execute;
    };

    class SlokedImmediateAsyncTask : public SlokedAsyncTask {
        using Callback = std::function<bool()>;
     public:
        SlokedImmediateAsyncTask(Callback);
        void Wait(std::function<void()>) final;
        bool Run() final;

     private:
        Callback callback;
    };

    class SlokedEventLoop {
     public:
        virtual ~SlokedEventLoop() = default;
        virtual void Attach(std::unique_ptr<SlokedAsyncTask>) = 0;
        virtual bool HasPending() const = 0;
        virtual void Run() = 0;
        virtual void SetListener(std::function<void()>) = 0;
    };

    class SlokedDefaultEventLoop : public SlokedEventLoop {
     public:
        SlokedDefaultEventLoop();
        void Attach(std::unique_ptr<SlokedAsyncTask>) final;
        bool HasPending() const final;
        void Run() final;
        void SetListener(std::function<void()>) final;
    
     private:
        mutable std::recursive_mutex mtx;
        std::queue<std::unique_ptr<SlokedAsyncTask>> pending;
        int64_t nextId;
        std::map<int64_t, std::unique_ptr<SlokedAsyncTask>> deferred;
        std::function<void()> callback;
    };
}

#endif