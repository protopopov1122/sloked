/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#include "sloked/editor/EditorServer.h"

namespace sloked {

    SlokedLocalEditorServer::SlokedLocalEditorServer()
        : unrestrictedServer(rawServer), server(unrestrictedServer, std::make_unique<SlokedNamedBlacklist>(), std::make_unique<SlokedNamedBlacklist>()) {}

    KgrNamedServer &SlokedLocalEditorServer::GetServer() {
        return this->server;
    }

    SlokedNamedRestrictionTarget &SlokedLocalEditorServer::GetRestrictions() {
        return this->server;
    }

    void SlokedLocalEditorServer::Start() {}
    
    void SlokedLocalEditorServer::Close() {}

    SlokedRemoteEditorServer::SlokedRemoteEditorServer(std::unique_ptr<SlokedSocket> socket, SlokedIOPoller &io, SlokedAuthenticatorFactory *authFactory)
        : unrestrictedServer(std::move(socket), io, authFactory), server(unrestrictedServer, std::make_unique<SlokedNamedBlacklist>(), std::make_unique<SlokedNamedBlacklist>()), deferredAuth{} {}

    SlokedRemoteEditorServer::~SlokedRemoteEditorServer() {
        this->Close();
    }

    KgrNamedServer &SlokedRemoteEditorServer::GetServer() {
        return this->server;
    }

    SlokedNamedRestrictionTarget &SlokedRemoteEditorServer::GetRestrictions() {
        return this->server;
    }

    void SlokedRemoteEditorServer::Start() {
        this->unrestrictedServer.Start();
        if (this->deferredAuth.has_value()) {
            this->unrestrictedServer.Authorize(this->deferredAuth.value());
            this->deferredAuth.reset();
        }
    }

    void SlokedRemoteEditorServer::Close() {
        this->unrestrictedServer.Close();
    }

    void SlokedRemoteEditorServer::Authorize(const std::string &user) {
        if (this->unrestrictedServer.IsRunning()) {
            this->unrestrictedServer.Authorize(user);
        } else {
            this->deferredAuth = user;
        }
    }
}