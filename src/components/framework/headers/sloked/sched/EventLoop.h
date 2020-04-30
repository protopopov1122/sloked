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

#ifndef SLOKED_SCHED_EVENTLOOP_H_
#define SLOKED_SCHED_EVENTLOOP_H_

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "sloked/core/Error.h"

namespace sloked {

    class SlokedDeferredTask {
     public:
        virtual ~SlokedDeferredTask() = default;
        virtual void Wait(std::function<void()>) = 0;
        virtual void Run() = 0;
    };

    class SlokedDynamicDeferredTask : public SlokedDeferredTask {
        using Callback =
            std::function<std::function<void()>(std::function<void()>)>;

     public:
        SlokedDynamicDeferredTask(Callback);
        void Wait(std::function<void()>) final;
        void Run() final;

     private:
        Callback callback;
        std::function<void()> execute;
    };

    class SlokedEventLoop {
     public:
        virtual ~SlokedEventLoop() = default;
        virtual void Attach(std::function<void()>) = 0;
        virtual void Attach(std::unique_ptr<SlokedDeferredTask>) = 0;
        virtual bool HasPending() const = 0;
        virtual void Notify(std::function<void()>) = 0;
    };

    class SlokedDefaultEventLoop : public SlokedEventLoop {
     public:
        SlokedDefaultEventLoop();
        void Attach(std::function<void()>) final;
        void Attach(std::unique_ptr<SlokedDeferredTask>) final;
        bool HasPending() const final;
        void Notify(std::function<void()>) final;
        void Run();

     private:
        mutable std::mutex mtx;
        std::vector<std::function<void()>> pending;
        int64_t nextId;
        std::map<int64_t, std::unique_ptr<SlokedDeferredTask>> deferred;
        std::function<void()> notification;
    };
}  // namespace sloked

#endif