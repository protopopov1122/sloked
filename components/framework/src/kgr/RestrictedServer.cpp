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
        : server(server), lifetime(std::make_shared<SlokedStandardLifetime>()),
          accessRestrictions(std::move(access)),
          modificationRestrictions(std::move(modification)) {}

    KgrRestrictedNamedServer::~KgrRestrictedNamedServer() {
        this->lifetime->Close();
    }

    void KgrRestrictedNamedServer::SetAccessRestrictions(
        std::shared_ptr<SlokedNamedRestrictions> restrictions) {
        this->accessRestrictions = std::move(restrictions);
    }

    void KgrRestrictedNamedServer::SetModificationRestrictions(
        std::shared_ptr<SlokedNamedRestrictions> restrictions) {
        this->modificationRestrictions = std::move(restrictions);
    }

    TaskResult<std::unique_ptr<KgrPipe>> KgrRestrictedNamedServer::Connect(
        const SlokedPath &name) {
        if (this->accessRestrictions != nullptr &&
            this->accessRestrictions->IsAllowed(name)) {
            return this->server.Connect(name);
        } else {
            TaskResultSupplier<std::unique_ptr<KgrPipe>> supplier;
            supplier.Wrap([&]() -> std::unique_ptr<KgrPipe> {
                throw SlokedError("KgrNamedServer: Connection to \'" +
                                  name.ToString() + "\' restricted");
            });
            return supplier.Result();
        }
    }

    KgrNamedServer::Connector KgrRestrictedNamedServer::GetConnector(
        const SlokedPath &name) {
        return [this, name] { return this->Connect(name); };
    }

    TaskResult<void> KgrRestrictedNamedServer::Register(
        const SlokedPath &name, std::unique_ptr<KgrService> service) {
        TaskResultSupplier<void> supplier;
        supplier.Catch([&] {
            if (this->modificationRestrictions != nullptr &&
                this->modificationRestrictions->IsAllowed(name)) {
                auto res = this->server.Register(name, std::move(service));
                res.Notify(
                    [supplier](const auto &result) {
                        switch (result.State()) {
                            case TaskResultStatus::Ready:
                                supplier.SetResult();
                                break;

                            case TaskResultStatus::Error:
                                supplier.SetError(result.GetError());
                                break;

                            case TaskResultStatus::Cancelled:
                                supplier.Cancel();
                                break;

                            default:
                                break;
                        }
                    },
                    this->lifetime);
            } else {
                throw SlokedError("KgrNamedServer: Modification restricted \'" +
                                  name.ToString() + "\'");
            }
        });
        return supplier.Result();
    }

    TaskResult<bool> KgrRestrictedNamedServer::Registered(
        const SlokedPath &name) {
        TaskResultSupplier<bool> supplier;
        supplier.Catch([&] {
            if ((this->accessRestrictions != nullptr &&
                 this->accessRestrictions->IsAllowed(name)) ||
                (this->modificationRestrictions != nullptr &&
                 this->modificationRestrictions->IsAllowed(name))) {
                this->server.Registered(name).Notify(
                    [supplier](const auto &result) {
                        switch (result.State()) {
                            case TaskResultStatus::Ready:
                                supplier.SetResult(result.GetResult());
                                break;

                            case TaskResultStatus::Error:
                                supplier.SetError(result.GetError());
                                break;

                            case TaskResultStatus::Cancelled:
                                supplier.Cancel();
                                break;

                            default:
                                break;
                        }
                    },
                    this->lifetime);
            } else {
                supplier.SetResult(false);
            }
        });
        return supplier.Result();
    }

    TaskResult<void> KgrRestrictedNamedServer::Deregister(
        const SlokedPath &name) {
        TaskResultSupplier<void> supplier;
        supplier.Catch([&] {
            if (this->modificationRestrictions != nullptr &&
                this->modificationRestrictions->IsAllowed(name)) {
                this->server.Deregister(name).Notify(
                    [supplier](const auto &result) {
                        switch (result.State()) {
                            case TaskResultStatus::Ready:
                                supplier.SetResult();
                                break;

                            case TaskResultStatus::Error:
                                supplier.SetError(result.GetError());
                                break;

                            case TaskResultStatus::Cancelled:
                                supplier.Cancel();
                                break;

                            default:
                                break;
                        }
                    },
                    this->lifetime);
            } else {
                throw SlokedError("KgrNamedServer: Modification restricted \'" +
                                  name.ToString() + "\'");
            }
        });
        return supplier.Result();
    }
}  // namespace sloked