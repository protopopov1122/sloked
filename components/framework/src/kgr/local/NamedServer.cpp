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

    TaskResult<void> KgrLocalNamedServer::Register(
        const SlokedPath &name, std::unique_ptr<KgrService> service) {
        TaskResultSupplier<void> supplier;
        try {
            auto absPath =
                name.IsAbsolute() ? name : name.RelativeTo(name.Root());
            std::unique_lock<std::mutex> lock(this->mtx);
            if (this->names.count(absPath) == 0) {
                this->server.Register(std::move(service))
                    .Notify([supplier, this, absPath](const auto &result) {
                        try {
                            this->names.emplace(absPath, result.Get());
                            supplier.SetResult();
                        } catch (...) {
                            supplier.SetError(std::current_exception());
                        }
                    });
            } else {
                throw SlokedError("KgrNamedServer: Name \'" + name.ToString() +
                                  "\' already exists");
            }
        } catch (...) { supplier.SetError(std::current_exception()); }
        return supplier.Result();
    }

    TaskResult<bool> KgrLocalNamedServer::Registered(const SlokedPath &name) {
        TaskResultSupplier<bool> supplier;
        try {
            auto absPath =
                name.IsAbsolute() ? name : name.RelativeTo(name.Root());
            std::unique_lock<std::mutex> lock(this->mtx);
            supplier.SetResult(this->names.count(absPath) != 0);
        } catch (...) { supplier.SetError(std::current_exception()); }
        return supplier.Result();
    }

    TaskResult<void> KgrLocalNamedServer::Deregister(const SlokedPath &name) {
        TaskResultSupplier<void> supplier;
        try {
            auto absPath =
                name.IsAbsolute() ? name : name.RelativeTo(name.Root());
            std::unique_lock<std::mutex> lock(this->mtx);
            if (this->names.count(absPath) != 0) {
                auto srvId = this->names.at(absPath);
                this->server.Deregister(srvId);
                this->names.erase(absPath);
                supplier.SetResult();
            } else {
                throw SlokedError("KgrNamedServer: Unknown name \'" +
                                  name.ToString() + "\'");
            }
        } catch (...) { supplier.SetError(std::current_exception()); }
        return supplier.Result();
    }
}  // namespace sloked