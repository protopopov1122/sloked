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

#ifndef SLOKED_KGR_NET_MASTERSERVER_H_
#define SLOKED_KGR_NET_MASTERSERVER_H_

#include "sloked/net/Socket.h"
#include "sloked/kgr/NamedServer.h"
#include "sloked/core/Counter.h"
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>

namespace sloked {

    class KgrMasterNetServer {
     public:
        KgrMasterNetServer(KgrNamedServer &, std::unique_ptr<SlokedServerSocket>);
        bool IsRunning() const;
        void Start();
        void Stop();

     private:        
        KgrNamedServer &server;
        std::unique_ptr<SlokedServerSocket> srvSocket;
        std::atomic<bool> work;
        SlokedCounter<std::size_t> workers;
        std::mutex mtx;
        std::condition_variable cv;
    };
}

#endif