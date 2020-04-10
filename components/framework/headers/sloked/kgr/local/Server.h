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

#ifndef SLOKED_KGR_LOCAL_SERVER_H_
#define SLOKED_KGR_LOCAL_SERVER_H_

#include <map>

#include "sloked/core/DynamicBitset.h"
#include "sloked/kgr/ContextManager.h"
#include "sloked/kgr/Server.h"
#include "sloked/kgr/local/Context.h"

namespace sloked {

    class KgrLocalServer : public KgrServer {
     public:
        KgrLocalServer();
        ~KgrLocalServer();
        TaskResult<std::unique_ptr<KgrPipe>> Connect(ServiceId) override;
        Connector GetConnector(ServiceId) override;

        TaskResult<ServiceId> Register(std::unique_ptr<KgrService>) override;
        TaskResult<void> Register(ServiceId,
                                  std::unique_ptr<KgrService>) override;
        TaskResult<bool> Registered(ServiceId) override;
        TaskResult<void> Deregister(ServiceId) override;

     private:
        std::map<ServiceId, std::unique_ptr<KgrService>> services;
        SlokedDynamicBitset serviceAllocator;
        std::shared_ptr<SlokedStandardLifetime> lifetime;
    };
}  // namespace sloked

#endif