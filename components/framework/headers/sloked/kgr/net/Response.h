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

#ifndef SLOKED_KGR_NET_RESPONSE_H_
#define SLOKED_KGR_NET_RESPONSE_H_

#include <list>
#include <mutex>
#include <queue>

#include "sloked/core/Closeable.h"
#include "sloked/kgr/Value.h"
#include "sloked/sched/Task.h"

namespace sloked {

    class SlokedNetResponseBroker : public SlokedCloseable {
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

        class Channel {
         public:
            using Id = int64_t;

            virtual ~Channel() = default;
            virtual Id GetID() const = 0;
            virtual TaskResult<Response> Next() = 0;
        };

        std::shared_ptr<Channel> OpenChannel();
        std::shared_ptr<Channel> GetChannel(Channel::Id) const;
        void DropChannel(Channel::Id);
        void Feed(Channel::Id, Response);
        void Close() final;

     private:
        class SimplexChannel : public Channel, public SlokedCloseable {
         public:
            SimplexChannel(SlokedNetResponseBroker &, int64_t);
            ~SimplexChannel();
            Id GetID() const final;
            TaskResult<Response> Next() final;
            void Push(Response);
            void Close() final;

         private:
            SlokedNetResponseBroker &broker;
            int64_t id;
            std::mutex mtx;
            std::list<Response> pending;
            std::list<std::pair<TaskResultSupplier<Response>,
                                std::shared_ptr<Channel>>>
                awaiting;
        };

        mutable std::mutex mtx;
        int64_t nextId{0};
        std::queue<KgrValue> incoming;
        std::map<int64_t, std::weak_ptr<SimplexChannel>> active;
    };
}  // namespace sloked

#endif