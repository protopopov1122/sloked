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

#include "sloked/editor/EditorApp.h"
#include "sloked/core/Error.h"
#include "sloked/kgr/net/Config.h"
#include "sloked/net/CryptoSocket.h"

namespace sloked {

    SlokedEditorApp::SlokedEditorApp(std::unique_ptr<SlokedIOPoll> ioPoll, SlokedSocketFactory &network)
        : ioPoll(std::move(ioPoll)), network{nullptr}, crypto{nullptr}, server{nullptr} {
        this->ioPoller = std::make_unique<SlokedDefaultIOPollThread>(*this->ioPoll);
        this->ioPoller->Start(KgrNetConfig::RequestTimeout);
        this->closeables.Attach(*this->ioPoller);
        this->sched.Start();
        this->closeables.Attach(this->sched);
        this->network = std::make_unique<SlokedNetworkFacade>(network);
    }

    SlokedCryptoFacade &SlokedEditorApp::InitializeCrypto(SlokedCrypto &crypto) {
        if (this->crypto != nullptr) {
            throw SlokedError("EditorApp: Crypto already initialized");
        } else {
            this->crypto = std::make_unique<SlokedCryptoFacade>(crypto);
            return *this->crypto;
        }
    }

    SlokedServerFacade &SlokedEditorApp::InitializeServer() {
        if (this->server != nullptr) {
            throw SlokedError("EditorApp: Server already initialized");
        } else {
            this->server = std::make_unique<SlokedServerFacade>(std::make_unique<SlokedLocalEditorServer>());
            this->closeables.Attach(*this->server);
            this->server->Start();
            return this->GetServer();
        }
    }

    SlokedServerFacade &SlokedEditorApp::InitializeServer(std::unique_ptr<SlokedSocket> socket, SlokedIOPoller &io, SlokedAuthenticatorFactory *auth) {
        if (this->server != nullptr) {
            throw SlokedError("EditorApp: Server already initialized");
        } else {
            this->server = std::make_unique<SlokedServerFacade>(std::make_unique<SlokedRemoteEditorServer>(std::move(socket), io, auth));
            this->closeables.Attach(*this->server);
            this->server->Start();
            return this->GetServer();
        }
    }

    void SlokedEditorApp::RequestStop() {
        this->termination.Notify();
    }

    void SlokedEditorApp::WaitForStop() {
        this->termination.WaitAll();
        this->closeables.Close();
        this->crypto = nullptr;
        this->server = nullptr;
    }

    void SlokedEditorApp::Attach(SlokedCloseable &closeable) {
        this->closeables.Attach(closeable);
    }

    SlokedCharWidth &SlokedEditorApp::GetCharWidth() {
        return this->charWidth;
    }

    SlokedSchedulerThread &SlokedEditorApp::GetScheduler() {
        return this->sched;
    }

    SlokedIOPoller &SlokedEditorApp::GetIO() {
        return *this->ioPoller;
    }

    SlokedNetworkFacade &SlokedEditorApp::GetNetwork() {
        return *this->network;
    }

    SlokedCryptoFacade &SlokedEditorApp::GetCrypto() {
        if (this->crypto) {
            return *this->crypto;
        } else {
            throw SlokedError("EditorApp: Crypto not defined");
        }
    }

    SlokedServerFacade &SlokedEditorApp::GetServer() {
        if (this->server) {
            return *this->server;
        } else {
            throw SlokedError("EditorApp: Server not defined");
        }
    }
}