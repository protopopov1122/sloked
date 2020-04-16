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

#ifndef SLOKED_KGR_PIPE_H_
#define SLOKED_KGR_PIPE_H_

#include <functional>
#include <mutex>
#include <queue>

#include "sloked/kgr/Value.h"
#include "sloked/sched/Task.h"

namespace sloked {

    class KgrPipe {
     public:
        enum class Status { Open, Closed };

        virtual ~KgrPipe() = default;
        virtual Status GetStatus() const = 0;
        virtual bool Empty() const = 0;
        virtual std::size_t Count() const = 0;
        virtual void Close() = 0;

        virtual KgrValue Read() = 0;
        virtual std::optional<KgrValue> ReadOptional() = 0;
        virtual KgrValue ReadWait() = 0;
        virtual void SetMessageListener(std::function<void()>) = 0;
        virtual bool Wait(std::size_t = 1) = 0;
        virtual void Drop(std::size_t = 1) = 0;
        virtual void DropAll() = 0;

        virtual void Write(KgrValue &&) = 0;
        virtual bool WriteNX(KgrValue &&) = 0;
    };

    class KgrAsyncPipe {
     public:
        KgrAsyncPipe(KgrPipe &);
        ~KgrAsyncPipe();
        KgrPipe::Status GetStatus() const;
        bool Empty() const;
        std::size_t Count() const;
        void Close();

        TaskResult<KgrValue> Read();
        TaskResult<void> Wait(std::size_t = 1);
        void SetMessageListener(std::function<void()>);
        void Drop(std::size_t = 1);
        void DropAll();

        void Write(KgrValue &&);
        bool WriteNX(KgrValue &&);

     private:
        void Notify();

        KgrPipe &pipe;
        std::mutex mtx;
        std::queue<std::function<void(std::unique_lock<std::mutex>)>> callbacks;
        std::function<void()> baseCallback;
    };
}  // namespace sloked

#endif