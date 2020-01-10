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

#ifndef SLOKED_FACADE_SERVER_H_
#define SLOKED_FACADE_SERVER_H_

#include "sloked/editor/EditorServer.h"
#include "sloked/kgr/net/MasterServer.h"
#include <variant>

namespace sloked {

    class SlokedServerFacade : public SlokedEditorServer {
     public:
        SlokedServerFacade(std::unique_ptr<SlokedLocalEditorServer>);
        SlokedServerFacade(std::unique_ptr<SlokedRemoteEditorServer>);

        bool IsLocal() const;
        bool IsRemote() const;
        bool HasNetServer() const;

        SlokedLocalEditorServer &AsLocalServer() const;
        SlokedRemoteEditorServer &AsRemoteServer() const;
        KgrMasterNetServer &GetNetworkServer() const;

        void SpawnNetServer(SlokedSocketFactory &, const SlokedSocketAddress &, SlokedIOPoller &, SlokedNamedRestrictionAuthority * = nullptr, SlokedAuthenticatorFactory * = nullptr);

        KgrNamedServer &GetServer() final;
        SlokedNamedRestrictionTarget &GetRestrictions() final;
        void Start() final;
        void Close() final;

     private:
        std::variant<std::unique_ptr<SlokedLocalEditorServer>, std::unique_ptr<SlokedRemoteEditorServer>> server;
        std::unique_ptr<KgrMasterNetServer> netServer;
    };
}

#endif
