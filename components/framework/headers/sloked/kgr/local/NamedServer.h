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

#ifndef SLOKED_KGR_LOCAL_NAMEDSERVER_H_
#define SLOKED_KGR_LOCAL_NAMEDSERVER_H_

#include <mutex>

#include "sloked/kgr/NamedServer.h"

namespace sloked {

    class KgrLocalNamedServer : public KgrNamedServer {
     public:
        KgrLocalNamedServer(KgrServer &);
        std::unique_ptr<KgrPipe> Connect(const SlokedPath &) override;
        Connector GetConnector(const SlokedPath &) override;

        void Register(const SlokedPath &, std::unique_ptr<KgrService>) override;
        bool Registered(const SlokedPath &) override;
        void Deregister(const SlokedPath &) override;

     private:
        KgrServer &server;
        std::map<SlokedPath, KgrServer::ServiceId> names;
        std::mutex mtx;
    };
}  // namespace sloked

#endif