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

#include "sloked/facade/Server.h"

namespace sloked {

    SlokedServerFacade::SlokedServerFacade(
        std::unique_ptr<SlokedLocalEditorServer> srv)
        : server(std::move(srv)) {}

    SlokedServerFacade::SlokedServerFacade(
        std::unique_ptr<SlokedRemoteEditorServer> srv)
        : server(std::move(srv)) {}

    bool SlokedServerFacade::IsLocal() const {
        return this->server.index() == 0;
    }

    bool SlokedServerFacade::IsRemote() const {
        return this->server.index() == 1;
    }

    bool SlokedServerFacade::HasNetServer() const {
        return this->netServer != nullptr;
    }

    SlokedLocalEditorServer &SlokedServerFacade::AsLocalServer() const {
        if (this->IsLocal()) {
            return *std::get<0>(this->server);
        } else {
            throw SlokedError("ServerFacade: Not a local server");
        }
    }

    SlokedRemoteEditorServer &SlokedServerFacade::AsRemoteServer() const {
        if (this->IsRemote()) {
            return *std::get<1>(this->server);
        } else {
            throw SlokedError("ServerFacade: Not a remote server");
        }
    }

    KgrMasterNetServer &SlokedServerFacade::GetNetworkServer() const {
        if (this->netServer != nullptr) {
            return *this->netServer;
        } else {
            throw SlokedError("ServerFacade: Net server not defined");
        }
    }

    void SlokedServerFacade::SpawnNetServer(
        SlokedSocketFactory &socketFactory, SlokedScheduler &sched,
        const SlokedSocketAddress &addr, SlokedIOPoller &io,
        SlokedNamedRestrictionAuthority *restrictions,
        SlokedAuthenticatorFactory *authFactory) {
        if (this->netServer == nullptr) {
            this->netServer = std::make_unique<KgrMasterNetServer>(
                this->GetServer(), socketFactory.Bind(addr), io, sched,
                restrictions, authFactory);
        } else {
            throw SlokedError("Editor: network server already spawned");
        }
    }

    KgrNamedServer &SlokedServerFacade::GetServer() {
        if (this->IsLocal()) {
            return this->AsLocalServer().GetServer();
        } else {
            return this->AsRemoteServer().GetServer();
        }
    }

    SlokedNamedRestrictionTarget &SlokedServerFacade::GetRestrictions() {
        if (this->IsLocal()) {
            return this->AsLocalServer().GetRestrictions();
        } else {
            return this->AsRemoteServer().GetRestrictions();
        }
    }

    void SlokedServerFacade::Start() {
        if (this->IsLocal()) {
            this->AsLocalServer().Start();
        } else {
            this->AsRemoteServer().Start();
        }
        if (this->HasNetServer()) {
            this->netServer->Start();
        }
    }

    void SlokedServerFacade::Close() {
        if (this->HasNetServer()) {
            this->netServer->Close();
        }
        if (this->IsLocal()) {
            this->AsLocalServer().Close();
        } else {
            this->AsRemoteServer().Close();
        }
    }
}  // namespace sloked