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

    std::unique_ptr<KgrPipe> KgrLocalServer::Connect(ServiceId srvId) {
        if (this->services.count(srvId) == 0) {
            throw SlokedError("KgrServer: Unknown service #" + std::to_string(srvId));
        }
        auto [clientPipe, servicePipe] = KgrLocalPipe::Make();
        if (this->services.at(srvId)->Attach(std::move(servicePipe))) {
            return std::move(clientPipe);
        } else {
            return nullptr;
        }
    }

    KgrLocalServer::ServiceId KgrLocalServer::Register(std::unique_ptr<KgrService> service) {
        if (service == nullptr) {
            throw SlokedError("KgrServer: Service can't be null");
        }
        auto serviceId = this->serviceAllocator.Allocate();
        this->services.emplace(serviceId, std::move(service));
        return serviceId;
    }

    void KgrLocalServer::Register(ServiceId serviceId, std::unique_ptr<KgrService> service) {
        if (service == nullptr) {
            throw SlokedError("KgrServer: Service can't be null");
        }
        if (this->services.count(serviceId) == 0) {
            this->serviceAllocator.Set(serviceId, true);
            this->services.emplace(serviceId, std::move(service));
        } else {
            throw SlokedError("KgrLocalServer: Sevice #" + std::to_string(serviceId) + " already allocated");
        }
    }

    bool KgrLocalServer::Registered(ServiceId serviceId) {
        return this->services.count(serviceId) != 0;
    }

    void KgrLocalServer::Deregister(ServiceId serviceId) {
        if (this->services.count(serviceId)) {
            this->serviceAllocator.Set(serviceId, false);
            this->services.erase(serviceId);
        } else {
            throw SlokedError("KgrServer: Unknown service #" + std::to_string(serviceId));
        }
    }
}