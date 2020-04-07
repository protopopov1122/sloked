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

#ifndef SLOKED_KGR_NET_INTERFACE_H_
#define SLOKED_KGR_NET_INTERFACE_H_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <queue>

#include "sloked/core/RingBuffer.h"
#include "sloked/kgr/Value.h"
#include "sloked/net/Socket.h"
#include "sloked/sched/Task.h"

namespace sloked {

    class KgrNetInterface {
     public:
        class Response {
         public:
            Response(const KgrValue &, bool);
            bool HasResult() const;
            const KgrValue &GetResult() const;
            const KgrValue &GetError() const;

         private:
            KgrValue content;
            bool result;
        };

        class InvokeResult {
         public:
            ~InvokeResult();
            TaskResult<Response> Next();

            friend class KgrNetInterface;

         private:
            InvokeResult(KgrNetInterface &, int64_t);
            void Push(Response);

            KgrNetInterface &net;
            int64_t id;
            std::mutex mtx;
            std::queue<Response> pending;
            std::queue<std::pair<TaskResultSupplier<Response>,
                                 std::shared_ptr<InvokeResult>>>
                awaiting;
        };

        friend class InvokeResult;

        class Responder {
         public:
            Responder(KgrNetInterface &, int64_t);
            void Result(const KgrValue &);
            void Error(const KgrValue &);

         private:
            KgrNetInterface &net;
            int64_t id;
        };
        friend class Responder;

        KgrNetInterface(std::unique_ptr<SlokedSocket>);
        virtual ~KgrNetInterface() = default;
        bool Wait(std::chrono::system_clock::duration =
                      std::chrono::system_clock::duration::zero()) const;
        std::size_t Available() const;
        bool Valid() const;
        void Receive();
        void Process(std::size_t = 0);
        std::shared_ptr<InvokeResult> Invoke(const std::string &,
                                             const KgrValue &);
        void Close();
        void BindMethod(const std::string &,
                        std::function<void(const std::string &,
                                           const KgrValue &, Responder &)>);
        std::unique_ptr<SlokedIOAwaitable> Awaitable() const;
        SlokedSocketEncryption *GetEncryption();

     protected:
        virtual void InvokeMethod(const std::string &, const KgrValue &,
                                  Responder &);

     private:
        void Write(const KgrValue &);
        void ActionInvoke(const KgrValue &);
        void ActionResponse(const KgrValue &);
        void ActionClose(const KgrValue &);

        std::shared_ptr<InvokeResult> NewResult();
        void DropResult(int64_t);

        std::unique_ptr<SlokedSocket> socket;
        SlokedRingBuffer<uint8_t, SlokedRingBufferType::Dynamic> buffer;
        std::mutex mtx;
        int64_t nextId;
        std::queue<KgrValue> incoming;
        std::map<int64_t, std::weak_ptr<InvokeResult>> responses;
        std::map<std::string,
                 std::function<void(const std::string &, const KgrValue &,
                                    Responder &)>>
            methods;
        std::mutex write_mtx;
    };
}  // namespace sloked

#endif