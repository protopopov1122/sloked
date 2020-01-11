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
        : running{false}, ioPoll(std::move(ioPoll)), network(network) {
        this->ioPoller = std::make_unique<SlokedDefaultIOPollThread>(*this->ioPoll);
        this->closeables.Attach(*this->ioPoller);
        this->closeables.Attach(this->sched);
    }

    SlokedCryptoFacade &SlokedEditorApp::InitializeCrypto(SlokedCrypto &crypto) {
        if (this->running.load()) {
            throw SlokedError("EditorApp: Already running");
        } else if (this->crypto != nullptr) {
            throw SlokedError("EditorApp: Crypto already initialized");
        } else {
            this->crypto = std::make_unique<SlokedCryptoFacade>(crypto);
            return *this->crypto;
        }
    }

    SlokedServerFacade &SlokedEditorApp::InitializeServer() {
        if (this->running.load()) {
            throw SlokedError("EditorApp: Already running");
        } else if (this->server != nullptr) {
            throw SlokedError("EditorApp: Server already initialized");
        } else {
            this->server = std::make_unique<SlokedServerFacade>(std::make_unique<SlokedLocalEditorServer>());
            this->closeables.Attach(*this->server);
            if (this->services) {
                this->services->Apply(this->server->GetServer());
            }
            return *this->server;
        }
    }

    SlokedServerFacade &SlokedEditorApp::InitializeServer(std::unique_ptr<SlokedSocket> socket) {
        if (this->running.load()) {
            throw SlokedError("EditorApp: Already running");
        } else if (this->server != nullptr) {
            throw SlokedError("EditorApp: Server already initialized");
        } else {
            SlokedAuthenticatorFactory *auth{nullptr};
            if (this->crypto && this->crypto->HasAuthenticator()) {
                auth = std::addressof(this->crypto->GetAuthenticator());
            }
            this->server = std::make_unique<SlokedServerFacade>(std::make_unique<SlokedRemoteEditorServer>(std::move(socket), *this->ioPoller, auth));
            this->closeables.Attach(*this->server);
            if (this->services) {
                this->services->Apply(this->server->GetServer());
            }
            return *this->server;
        }
    }

    SlokedAbstractServicesFacade &SlokedEditorApp::InitializeServices(std::unique_ptr<SlokedAbstractServicesFacade> services) {
        if (this->running.load()) {
            throw SlokedError("EditorApp: Already running");
        } else if (this->services != nullptr) {
            throw SlokedError("EditorApp: Services already initialized");
        } else {
            this->services = std::move(services);
            this->closeables.Attach(*this->services);
            if (this->server) {
                this->services->Apply(this->server->GetServer());
            }
            return *this->services;
        }
    }

    void SlokedEditorApp::Attach(SlokedCloseable &closeable) {
        this->closeables.Attach(closeable);
    }

    bool SlokedEditorApp::IsRunning() const {
        return this->running.load();
    }

    void SlokedEditorApp::Start() {
        if (!this->running.exchange(true)) {
            this->ioPoller->Start(KgrNetConfig::RequestTimeout);
            this->sched.Start();
            if (this->services) {
                this->services->Start();
            }
            if (this->server) {
                this->server->Start();
            }
        } else {
            throw SlokedError("EditorApp: Already running");
        }
    }

    void SlokedEditorApp::Stop() {
        if (this->running.load()) {
            std::thread([this] {
                std::unique_lock lock(this->termination_mtx);
                this->closeables.Close();
                this->server = nullptr;
                this->services = nullptr;
                this->crypto = nullptr;
                this->running = false;
                this->termination_cv.notify_all();
            }).detach();
        } else {
            throw SlokedError("EditorApp: Not running");
        }
    }

    void SlokedEditorApp::Wait() {
        std::unique_lock lock(this->termination_mtx);
        while (this->running.load()) {
            this->termination_cv.wait(lock);
        }
    }

    void SlokedEditorApp::Close() {
        this->Stop();
        this->Wait();
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
        return this->network;
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