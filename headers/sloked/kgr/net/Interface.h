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

#ifndef SLOKED_KGR_NET_INTERFACE_H_
#define SLOKED_KGR_NET_INTERFACE_H_

#include "sloked/kgr/Value.h"
#include "sloked/net/Socket.h"
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>

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

        class ResponseHandle {
         public:
            ResponseHandle(KgrNetInterface &, int64_t);
            ResponseHandle(const ResponseHandle &) = delete;
            ResponseHandle(ResponseHandle &&) = default;
            ~ResponseHandle();
            ResponseHandle &operator=(const ResponseHandle &) = delete;
            ResponseHandle &operator=(ResponseHandle &&) = default;
            bool HasResponse() const;
            bool WaitResponse(long) const;
            Response GetResponse() const;

         private:
            KgrNetInterface *net;
            int64_t id;
        };
        friend class ResponseHandle;

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
        bool Wait(long = 0) const;
        bool Valid() const;
        void Receive();
        void Process(std::size_t = 0);
        ResponseHandle Invoke(const std::string &, const KgrValue &);
        void Close();
        void BindMethod(const std::string &, std::function<void(const std::string &, const KgrValue &, Responder &)>);

     protected:
        virtual void InvokeMethod(const std::string &, const KgrValue &, Responder &);

     private:
        void Write(const KgrValue &);
        void ActionInvoke(const KgrValue &);
        void ActionResponse(const KgrValue &);
        void ActionClose(const KgrValue &);

        std::unique_ptr<SlokedSocket> socket;
        std::string buffer;
        int64_t nextId;
        std::queue<KgrValue> incoming;
        std::map<int64_t, std::queue<Response>> responses;
        std::mutex response_mtx;
        std::condition_variable response_cv;
        std::map<std::string, std::function<void(const std::string &, const KgrValue &, Responder &)>> methods;
    };
}

#endif