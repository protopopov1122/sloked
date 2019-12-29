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

#include "sloked/core/Closeable.h"
#include "sloked/net/Socket.h"
#include "sloked/core/awaitable/Poll.h"
#include "sloked/kgr/NamedServer.h"
#include "sloked/core/Counter.h"
#include "sloked/kgr/local/Server.h"
#include "sloked/kgr/local/NamedServer.h"
#include "sloked/kgr/RestrictedServer.h"
#include "sloked/security/Authenticator.h"
#include <atomic>
#include <vector>

namespace sloked {

    class KgrMasterNetServer : public SlokedCloseable {
     public:
        KgrMasterNetServer(KgrNamedServer &, std::unique_ptr<SlokedServerSocket>, SlokedIOPoller &, SlokedNamedRestrictionAuthority &, SlokedAuthenticatorFactory &);
        ~KgrMasterNetServer();
        bool IsRunning() const;
        void Start();
        void Close() final;

     private:
        class Awaitable : public SlokedIOPoller::Awaitable {
         public:
            Awaitable(KgrMasterNetServer &);
            std::unique_ptr<SlokedIOAwaitable> GetAwaitable() const final;
            void Process(bool) final;

         private:
            KgrMasterNetServer &self;
        };
        friend class Awaitable;

        KgrNamedServer &server;
        KgrLocalServer rawRemoteServices;
        KgrLocalNamedServer remoteServices;
        std::unique_ptr<SlokedServerSocket> srvSocket;
        SlokedIOPoller &poll;
        SlokedNamedRestrictionAuthority &restrictions;
        SlokedAuthenticatorFactory &authFactory;
        SlokedIOPoller::Handle awaiterHandle;
        std::atomic<bool> work;
        SlokedCounter<std::size_t> workers;
    };
}

#endif