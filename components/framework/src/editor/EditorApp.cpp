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

    SlokedEditorApp::Crypto::Crypto(SlokedCrypto &crypto)
        : crypto(crypto), credentials{std::unique_ptr<SlokedCredentialMaster>(nullptr)}, authenticator{} {}

    SlokedCrypto &SlokedEditorApp::Crypto::GetEngine() const {
        return this->crypto;
    }

    bool SlokedEditorApp::Crypto::HasCredentialMaster() const {
        return this->credentials.index() == 0 && std::get<0>(this->credentials) != nullptr;
    }

    SlokedCredentialMaster &SlokedEditorApp::Crypto::GetCredentialMaster() const {
        if (this->HasCredentialMaster()) {
            return *std::get<0>(this->credentials);
        } else {
            throw SlokedError("EditorAppCrypto: Credential master not defined");
        }
    }

    bool SlokedEditorApp::Crypto::HasCredentialSlave() const {
        return this->credentials.index() == 1 && std::get<1>(this->credentials) != nullptr;
    }

    SlokedCredentialSlave &SlokedEditorApp::Crypto::GetCredentialSlave() const {
        if (this->HasCredentialSlave()) {
            return *std::get<1>(this->credentials);
        } else {
            throw SlokedError("EditorAppCrypto: Credential slave not defined");
        }
    }

    bool SlokedEditorApp::Crypto::HasAuthenticator() const {
        return this->authenticator != nullptr;
    }

    SlokedAuthenticatorFactory &SlokedEditorApp::Crypto::GetAuthenticator() const {
        if (this->HasAuthenticator()) {
            return *this->authenticator;
        } else {
            throw SlokedError("EditorAppCrypto: Authenticator not defined");
        }
    }

    SlokedCredentialMaster &SlokedEditorApp::Crypto::SetupCredentialMaster(SlokedCrypto::Key &key) {
        auto master = std::make_unique<SlokedCredentialMaster>(this->crypto, key);
        auto &masterRef = *master;
        this->authenticator = nullptr;
        this->credentials = std::move(master);
        return masterRef;
    }

    SlokedCredentialSlave &SlokedEditorApp::Crypto::SetupCredentialSlave() {
        auto slave = std::make_unique<SlokedCredentialSlave>(this->crypto);
        auto &slaveRef = *slave;
        this->authenticator = nullptr;
        this->credentials = std::move(slave);
        return slaveRef;
    }

    SlokedAuthenticatorFactory &SlokedEditorApp::Crypto::SetupAuthenticator(const std::string &salt) {
        if (this->HasCredentialMaster()) {
            this->authenticator = std::make_unique<SlokedAuthenticatorFactory>(this->crypto, this->GetCredentialMaster(), salt);
        } else if (this->HasCredentialSlave()) {
            this->authenticator = std::make_unique<SlokedAuthenticatorFactory>(this->crypto, this->GetCredentialSlave(), salt);
        } else {
            throw SlokedError("EditorAppCrypto: Credentials not defined");
        }
        return *this->authenticator;
    }

    SlokedEditorApp::Network::Network(SlokedEditorApp &app, SlokedSocketFactory &baseEngine)
        : app(app), baseEngine(baseEngine), engine{} {}

    SlokedSocketFactory &SlokedEditorApp::Network::GetEngine() const {
        if (this->engine) {
            return *this->engine;
        } else {
            return this->baseEngine;
        }
    }

    void SlokedEditorApp::Network::SetupCrypto(SlokedCrypto::Key &key) {
        this->engine = std::make_unique<SlokedCryptoSocketFactory>(this->baseEngine, this->app.GetCrypto().GetEngine(), key);
    }

    SlokedEditorApp::SlokedEditorApp()
        : running(false), network{nullptr}, crypto{nullptr}, server{std::unique_ptr<SlokedEditorMasterCore>(nullptr)} {}

    bool SlokedEditorApp::IsRunning() const {
        return this->running.load();
    }

    void SlokedEditorApp::Initialize(std::unique_ptr<SlokedIOPoll> ioPoll, SlokedSocketFactory &network) {
        if (this->running.exchange(true)) {
            throw SlokedError("EditorApp: Already initialized");
        }
        this->ioPoll = std::move(ioPoll);
        this->ioPoller = std::make_unique<SlokedDefaultIOPollThread>(*this->ioPoll);
        this->ioPoller->Start(KgrNetConfig::RequestTimeout);
        this->closeables.Attach(*this->ioPoller);
        this->sched.Start();
        this->closeables.Attach(this->sched);
        this->network = std::make_unique<Network>(*this, network);
    }

    SlokedEditorApp::Crypto &SlokedEditorApp::InitializeCrypto(SlokedCrypto &crypto) {
        if (!this->running.load()) {
            throw SlokedError("EditorApp: Not running");
        } else if (this->crypto != nullptr) {
            throw SlokedError("EditorApp: Crypto already initialized");
        } else {
            this->crypto = std::make_unique<Crypto>(crypto);
            return *this->crypto;
        }
    }

    bool SlokedEditorApp::HasMasterServer() const {
        return this->server.index() == 0 && std::get<0>(this->server) != nullptr;
    }

    SlokedEditorMasterCore &SlokedEditorApp::GetMasterServer() const {
        if (this->HasMasterServer()) {
            return *std::get<0>(this->server);
        } else {
            throw SlokedError("EditorApp: Master server not defined");
        }
    }

    SlokedEditorMasterCore &SlokedEditorApp::InitializeMasterServer(SlokedLogger &logger, SlokedMountableNamespace &root, SlokedNamespaceMounter &mounter) {
        if (!this->running.load()) {
            throw SlokedError("EditorApp: Not running");
        } else if (this->HasMasterServer() || this->HasSlaveServer()) {
            throw SlokedError("EditorApp: Server already initialized");
        } else {
            this->server = std::make_unique<SlokedEditorMasterCore>(logger, this->GetIO(), root, mounter, this->GetCharWidth());
            auto &srv = *std::get<0>(this->server);
            this->closeables.Attach(srv);
            srv.Start();
            return srv;
        }
    }

    bool SlokedEditorApp::HasSlaveServer() const {
        return this->server.index() == 1 && std::get<1>(this->server) != nullptr;
    }

    void SlokedEditorApp::RequestStop() {
        if (!this->running.load()) {
            throw SlokedError("EditorApp: Not running");
        } else {
            this->termination.Notify();
        }
    }

    void SlokedEditorApp::WaitForStop() {
        this->termination.WaitAll();
        this->closeables.Close();
        this->crypto = nullptr;
        this->running = false;
    }

    void SlokedEditorApp::Attach(SlokedCloseable &closeable) {
        if (!this->running.load()) {
            throw SlokedError("EditorApp: Not running");
        } else {
            this->closeables.Attach(closeable);
        }
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

    SlokedEditorApp::Network &SlokedEditorApp::GetNetwork() {
        return *this->network;
    }

    SlokedEditorApp::Crypto &SlokedEditorApp::GetCrypto() {
        if (this->crypto) {
            return *this->crypto;
        } else {
            throw SlokedError("EditorApp: Crypto not defined");
        }
    }
}