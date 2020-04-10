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
#include "sloked/kgr/net/Response.h"
#include "sloked/net/Socket.h"
#include "sloked/sched/DelayManager.h"
#include "sloked/sched/Scheduler.h"
#include "sloked/sched/Task.h"

namespace sloked {

    class KgrNetInterface {
     public:
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

        KgrNetInterface(std::unique_ptr<SlokedSocket>, SlokedScheduler &);
        virtual ~KgrNetInterface() = default;
        bool Wait(std::chrono::system_clock::duration =
                      std::chrono::system_clock::duration::zero()) const;
        std::size_t Available() const;
        bool Valid() const;
        void Receive();
        void Process(std::size_t = 0);
        std::shared_ptr<SlokedNetResponseBroker::Channel> Invoke(
            const std::string &, const KgrValue &);
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

        std::unique_ptr<SlokedSocket> socket;
        SlokedRingBuffer<uint8_t, SlokedRingBufferType::Dynamic> buffer;
        SlokedNetResponseBroker responseBroker;
        std::queue<KgrValue> incoming;
        std::map<std::string,
                 std::function<void(const std::string &, const KgrValue &,
                                    Responder &)>>
            methods;
        std::mutex write_mtx;
        SlokedDelayManager<SlokedNetResponseBroker::Channel::Id>
            inactivityTimeouts;
    };
}  // namespace sloked

#endif