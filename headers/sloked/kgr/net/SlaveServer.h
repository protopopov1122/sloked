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

#ifndef SLOKED_KGR_NET_SLAVESERVER_H_
#define SLOKED_KGR_NET_SLAVESERVER_H_

#include "sloked/core/Counter.h"
#include "sloked/net/Poll.h"
#include "sloked/kgr/net/Interface.h"
#include "sloked/kgr/NamedServer.h"
#include <mutex>
#include <atomic>

namespace sloked {

    class KgrSlaveNetServer : public KgrNamedServer {
     public:
        KgrSlaveNetServer(std::unique_ptr<SlokedSocket>, KgrNamedServer &, SlokedSocketPoller &);
        ~KgrSlaveNetServer();
        bool IsRunning() const;
        void Start();
        void Stop();

        std::unique_ptr<KgrPipe> Connect(const std::string &) override;
        Connector GetConnector(const std::string &) override;
        
        void Register(const std::string &, std::unique_ptr<KgrService>) override;
        bool Registered(const std::string &) override;
        void Deregister(const std::string &) override;

     private:
        void Accept();

        class Awaitable : public SlokedSocketPoller::Awaitable {
         public:
            Awaitable(KgrSlaveNetServer &);
            std::unique_ptr<SlokedSocketAwaitable> GetAwaitable() const final;
            void Process(bool) final;

         private:
            KgrSlaveNetServer &self;
        };
        friend class Awaitable;

        KgrNetInterface net;
        std::atomic<bool> work;
        SlokedSocketPoller::Handle awaitableHandle;
        std::recursive_mutex mtx;
        std::mutex send_mtx;
        SlokedCounter<std::size_t> workers;
        std::map<int64_t, std::unique_ptr<KgrPipe>> pipes;
        KgrNamedServer &localServer;
        SlokedSocketPoller &poll;
    };
}

#endif