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

#ifndef SLOKED_EDITOR_EDITORSERVER_H_
#define SLOKED_EDITOR_EDITORSERVER_H_

#include "sloked/core/Closeable.h"
#include "sloked/kgr/NamedServer.h"
#include "sloked/kgr/RestrictedServer.h"
#include "sloked/kgr/local/NamedServer.h"
#include "sloked/kgr/local/Server.h"
#include "sloked/kgr/net/SlaveServer.h"

namespace sloked {

    class SlokedEditorServer : public SlokedCloseable {
     public:
        virtual ~SlokedEditorServer() = default;
        virtual KgrNamedServer &GetServer() = 0;
        virtual SlokedNamedRestrictionTarget &GetRestrictions() = 0;
        virtual void Start() = 0;
    };

    class SlokedLocalEditorServer : public SlokedEditorServer {
     public:
        SlokedLocalEditorServer();
        KgrNamedServer &GetServer() final;
        SlokedNamedRestrictionTarget &GetRestrictions() final;
        void Start() final;
        void Close() final;

     private:
        KgrLocalServer rawServer;
        KgrLocalNamedServer unrestrictedServer;
        KgrRestrictedNamedServer server;
    };

    class SlokedRemoteEditorServer : public SlokedEditorServer {
     public:
        SlokedRemoteEditorServer(std::unique_ptr<SlokedSocket>,
                                 SlokedIOPoller &,
                                 SlokedAuthenticatorFactory *);
        ~SlokedRemoteEditorServer();
        KgrNamedServer &GetServer() final;
        SlokedNamedRestrictionTarget &GetRestrictions() final;
        void Start() final;
        void Close() final;
        void Authorize(const std::string &);

     private:
        KgrSlaveNetServer unrestrictedServer;
        KgrRestrictedNamedServer server;
        std::optional<std::string> deferredAuth;
    };
}  // namespace sloked

#endif