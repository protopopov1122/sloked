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

#include "sloked/kgr/local/NamedServer.h"

#include "sloked/core/Error.h"

namespace sloked {

    KgrLocalNamedServer::KgrLocalNamedServer(KgrServer &server)
        : server(server) {}

    std::unique_ptr<KgrPipe> KgrLocalNamedServer::Connect(
        const SlokedPath &name) {
        auto absPath = name.IsAbsolute() ? name : name.RelativeTo(name.Root());
        std::unique_lock<std::mutex> lock(this->mtx);
        if (this->names.count(absPath) != 0) {
            return this->server.Connect(this->names.at(absPath));
        } else {
            throw SlokedError("KgrNamedServer: Unknown name \'" +
                              name.ToString() + "\'");
        }
    }

    KgrLocalNamedServer::Connector KgrLocalNamedServer::GetConnector(
        const SlokedPath &name) {
        std::unique_lock<std::mutex> lock(this->mtx);
        return [this, name]() { return this->Connect(name); };
    }

    void KgrLocalNamedServer::Register(const SlokedPath &name,
                                       std::unique_ptr<KgrService> service) {
        auto absPath = name.IsAbsolute() ? name : name.RelativeTo(name.Root());
        std::unique_lock<std::mutex> lock(this->mtx);
        if (this->names.count(absPath) == 0) {
            this->names.emplace(absPath,
                                this->server.Register(std::move(service)));
        } else {
            throw SlokedError("KgrNamedServer: Name \'" + name.ToString() +
                              "\' already exists");
        }
    }

    bool KgrLocalNamedServer::Registered(const SlokedPath &name) {
        auto absPath = name.IsAbsolute() ? name : name.RelativeTo(name.Root());
        std::unique_lock<std::mutex> lock(this->mtx);
        return this->names.count(absPath) != 0;
    }

    void KgrLocalNamedServer::Deregister(const SlokedPath &name) {
        auto absPath = name.IsAbsolute() ? name : name.RelativeTo(name.Root());
        std::unique_lock<std::mutex> lock(this->mtx);
        if (this->names.count(absPath) != 0) {
            auto srvId = this->names.at(absPath);
            this->server.Deregister(srvId);
            this->names.erase(absPath);
        } else {
            throw SlokedError("KgrNamedServer: Unknown name \'" +
                              name.ToString() + "\'");
        }
    }
}  // namespace sloked