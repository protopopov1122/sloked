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

#include "sloked/kgr/local/NamedServer.h"
#include "sloked/core/Error.h"

namespace sloked {

    KgrLocalNamedServer::KgrLocalNamedServer(KgrServer &server)
        : server(server) {}

    std::unique_ptr<KgrPipe> KgrLocalNamedServer::Connect(const std::string &name) {
        if (this->names.count(name) != 0) {
            return this->server.Connect(this->names.at(name));
        } else {
            throw SlokedError("KgrNamedServer: Unknown name \'" + name + "\'");
        }
    }
    
    void KgrLocalNamedServer::Register(const std::string &name, std::unique_ptr<KgrService> service) {
        if (this->names.count(name) == 0) {
            this->names[name] = this->server.Register(std::move(service));
        } else {
            throw SlokedError("KgrNamedServer: Name \'" + name + "\' already exists");
        }
    }

    bool KgrLocalNamedServer::Registered(const std::string &name) {
        return this->names.count(name) == 0;
    }

    void KgrLocalNamedServer::Deregister(const std::string &name) {
        if (this->names.count(name) != 0) {
            auto srvId = this->names.at(name);
            this->server.Deregister(srvId);
            this->names.erase(name);
        } else {
            throw SlokedError("KgrNamedServer: Unknown name \'" + name + "\'");
        }
    }
}