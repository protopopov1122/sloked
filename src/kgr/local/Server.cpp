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

#include "sloked/kgr/local/Server.h"
#include "sloked/kgr/local/Pipe.h"
#include "sloked/core/Error.h"

namespace sloked {

    KgrLocalServer::KgrLocalServer()
        : nextServiceId(0) {}

    std::unique_ptr<KgrPipe> KgrLocalServer::Connect(ServiceId srvId) {
        if (this->services.count(srvId) == 0) {
            throw SlokedError("KgrServer: Unknown service #" + std::to_string(srvId));
        }
        auto [clientPipe, servicePipe] = KgrLocalPipe::Make();
        auto context = this->services.at(srvId)->Attach(std::move(servicePipe));
        if (context) {
            this->contexts.push_back(std::move(context));
            return std::move(clientPipe);
        } else {
            return nullptr;
        }
    }

    KgrLocalServer::ServiceId KgrLocalServer::Bind(std::unique_ptr<KgrService> service) {
        auto serviceId = this->nextServiceId++;
        this->services.emplace(serviceId, std::move(service));
        return serviceId;
    }

    void KgrLocalServer::Unbind(ServiceId serviceId) {
        if (this->services.count(serviceId)) {
            this->services.erase(serviceId);
        } else {
            throw SlokedError("KgrServer: Unknown service #" + std::to_string(serviceId));
        }
    }
}