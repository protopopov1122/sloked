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

#include "sloked/editor/EditorServer.h"

namespace sloked {

    SlokedLocalEditorServer::SlokedLocalEditorServer()
        : server(rawServer) {}

    KgrNamedServer &SlokedLocalEditorServer::GetServer() {
        return this->server;
    }

    void SlokedLocalEditorServer::Start() {}
    
    void SlokedLocalEditorServer::Close() {}

    SlokedRemoteEditorServer::SlokedRemoteEditorServer(std::unique_ptr<SlokedSocket> socket, SlokedIOPoller &io)
        : server(std::move(socket), io) {}

    SlokedRemoteEditorServer::~SlokedRemoteEditorServer() {
        this->Close();
    }

    KgrNamedServer &SlokedRemoteEditorServer::GetServer() {
        return this->server;
    }

    void SlokedRemoteEditorServer::Start() {
        this->server.Start();
    }

    void SlokedRemoteEditorServer::Close() {
        this->server.Close();
    }
}