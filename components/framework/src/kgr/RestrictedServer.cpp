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

#include "sloked/kgr/RestrictedServer.h"

#include "sloked/core/Error.h"
#include "sloked/core/String.h"

namespace sloked {

    KgrRestrictedNamedServer::KgrRestrictedNamedServer(
        KgrNamedServer &server, std::shared_ptr<SlokedNamedRestrictions> access,
        std::shared_ptr<SlokedNamedRestrictions> modification)
        : server(server), accessRestrictions(std::move(access)),
          modificationRestrictions(std::move(modification)) {}

    void KgrRestrictedNamedServer::SetAccessRestrictions(
        std::shared_ptr<SlokedNamedRestrictions> restrictions) {
        this->accessRestrictions = std::move(restrictions);
    }

    void KgrRestrictedNamedServer::SetModificationRestrictions(
        std::shared_ptr<SlokedNamedRestrictions> restrictions) {
        this->modificationRestrictions = std::move(restrictions);
    }

    std::unique_ptr<KgrPipe> KgrRestrictedNamedServer::Connect(
        const SlokedPath &name) {
        if (this->accessRestrictions != nullptr &&
            this->accessRestrictions->IsAllowed(name)) {
            return this->server.Connect(name);
        } else {
            throw SlokedError("KgrNamedServer: Connection to \'" +
                              name.ToString() + "\' restricted");
        }
    }

    KgrNamedServer::Connector KgrRestrictedNamedServer::GetConnector(
        const SlokedPath &name) {
        return [this, name] { return this->Connect(name); };
    }

    TaskResult<void> KgrRestrictedNamedServer::Register(
        const SlokedPath &name, std::unique_ptr<KgrService> service) {
        TaskResultSupplier<void> supplier;
        try {
            if (this->modificationRestrictions != nullptr &&
                this->modificationRestrictions->IsAllowed(name)) {
                auto res = this->server.Register(name, std::move(service));
                res.Notify([supplier](const auto &result) {
                    switch (result.State()) {
                        case TaskResult<void>::Status::Ready:
                            supplier.SetResult();
                            break;

                        case TaskResult<void>::Status::Error:
                            supplier.SetError(result.GetError());
                            break;

                        case TaskResult<void>::Status::Cancelled:
                            supplier.Cancel();
                            break;
                    }
                });
            } else {
                throw SlokedError("KgrNamedServer: Modification restricted \'" +
                                  name.ToString() + "\'");
            }
        } catch (...) { supplier.SetError(std::current_exception()); }
        return supplier.Result();
    }

    TaskResult<bool> KgrRestrictedNamedServer::Registered(
        const SlokedPath &name) {
        TaskResultSupplier<bool> supplier;
        try {
            if ((this->accessRestrictions != nullptr &&
                 this->accessRestrictions->IsAllowed(name)) ||
                (this->modificationRestrictions != nullptr &&
                 this->modificationRestrictions->IsAllowed(name))) {
                this->server.Registered(name).Notify(
                    [supplier](const auto &result) {
                        switch (result.State()) {
                            case TaskResult<bool>::Status::Ready:
                                supplier.SetResult(result.GetResult());
                                break;

                            case TaskResult<bool>::Status::Error:
                                supplier.SetError(result.GetError());
                                break;

                            case TaskResult<bool>::Status::Cancelled:
                                supplier.Cancel();
                                break;
                        }
                    });
            } else {
                supplier.SetResult(false);
            }
        } catch (...) { supplier.SetError(std::current_exception()); }
        return supplier.Result();
    }

    TaskResult<void> KgrRestrictedNamedServer::Deregister(
        const SlokedPath &name) {
        TaskResultSupplier<void> supplier;
        try {
            if (this->modificationRestrictions != nullptr &&
                this->modificationRestrictions->IsAllowed(name)) {
                this->server.Deregister(name).Notify(
                    [supplier](const auto &result) {
                        switch (result.State()) {
                            case TaskResult<void>::Status::Ready:
                                supplier.SetResult();
                                break;

                            case TaskResult<void>::Status::Error:
                                supplier.SetError(result.GetError());
                                break;

                            case TaskResult<void>::Status::Cancelled:
                                supplier.Cancel();
                                break;
                        }
                    });
            } else {
                throw SlokedError("KgrNamedServer: Modification restricted \'" +
                                  name.ToString() + "\'");
            }
        } catch (...) { supplier.SetError(std::current_exception()); }
        return supplier.Result();
    }
}  // namespace sloked