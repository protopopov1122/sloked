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

#include "sloked/editor/EditorInstance.h"

#include "sloked/core/Error.h"
#include "sloked/kgr/net/Config.h"
#include "sloked/net/CryptoSocket.h"

namespace sloked {

    SlokedEditorInstance::SlokedEditorInstance(
        std::unique_ptr<SlokedIOPoll> ioPoll, SlokedSocketFactory &network)
        : running{false}, ioPoll(std::move(ioPoll)), network(network) {
        this->ioPoller =
            std::make_unique<SlokedDefaultIOPollThread>(*this->ioPoll);
        this->closeables.Attach(*this->ioPoller);
        this->closeables.Attach(this->contextManager);
    }

    SlokedCryptoFacade &SlokedEditorInstance::InitializeCrypto(
        SlokedCrypto &crypto) {
        if (this->running.load()) {
            throw SlokedError("EditorInstance: Already running");
        } else if (this->crypto != nullptr) {
            throw SlokedError("EditorInstance: Crypto already initialized");
        } else {
            this->crypto = std::make_unique<SlokedCryptoFacade>(crypto);
            return *this->crypto;
        }
    }

    SlokedServerFacade &SlokedEditorInstance::InitializeServer() {
        if (this->running.load()) {
            throw SlokedError("EditorInstance: Already running");
        } else if (this->server != nullptr) {
            throw SlokedError("EditorInstance: Server already initialized");
        } else {
            this->server = std::make_unique<SlokedServerFacade>(
                std::make_unique<SlokedLocalEditorServer>());
            this->closeables.Attach(*this->server);
            return *this->server;
        }
    }

    SlokedServerFacade &SlokedEditorInstance::InitializeServer(
        std::unique_ptr<SlokedSocket> socket) {
        if (this->running.load()) {
            throw SlokedError("EditorInstance: Already running");
        } else if (this->server != nullptr) {
            throw SlokedError("EditorInstance: Server already initialized");
        } else {
            SlokedAuthenticatorFactory *auth{nullptr};
            if (this->crypto && this->crypto->HasAuthenticator()) {
                auth = std::addressof(this->crypto->GetAuthenticator());
            }
            this->server = std::make_unique<SlokedServerFacade>(
                std::make_unique<SlokedRemoteEditorServer>(
                    std::move(socket), *this->ioPoller, auth));
            this->closeables.Attach(*this->server);
            return *this->server;
        }
    }

    SlokedServiceDependencyProvider &
        SlokedEditorInstance::InitializeServiceProvider(
            std::unique_ptr<SlokedServiceDependencyProvider> provider) {
        if (this->running.load()) {
            throw SlokedError("EditorInstance: Already running");
        } else if (this->serviceProvider != nullptr) {
            throw SlokedError(
                "EditorInstance: Service provider already initialized");
        } else {
            this->serviceProvider = std::move(provider);
            this->closeables.Attach(*this->serviceProvider);
            return *this->serviceProvider;
        }
    }

    SlokedScreenServer &SlokedEditorInstance::InitializeScreen(
        SlokedScreenProviderFactory &providers, const SlokedUri &uri) {
        if (this->running.load()) {
            throw SlokedError("EditorInstance: Already running");
        } else if (this->screen != nullptr) {
            throw SlokedError("EditorInstance: Screen already initialized");
        } else {
            this->screenProvider = providers.Make(uri, this->GetCharPreset());
            this->screen = std::make_unique<SlokedScreenServer>(
                this->GetServer().GetServer(), *this->screenProvider,
                this->GetContextManager());
            this->closeables.Attach(*this->screen);
            return *this->screen;
        }
    }

    void SlokedEditorInstance::Attach(SlokedCloseable &closeable) {
        this->closeables.Attach(closeable);
    }

    void SlokedEditorInstance::Attach(
        std::unique_ptr<SlokedDataHandle> handle) {
        this->handles.emplace_back(std::move(handle));
    }

    bool SlokedEditorInstance::IsRunning() const {
        return this->running.load();
    }

    void SlokedEditorInstance::Start() {
        if (!this->running.exchange(true)) {
            this->ioPoller->Start(KgrNetConfig::RequestTimeout);
            this->sched.Start();
            this->closeables.Attach(this->sched);
            this->contextManager.Start();
            if (this->server) {
                this->server->Start();
                if (this->server->IsRemote()) {
                    this->charPresetUpdater =
                        std::make_unique<SlokedCharPresetClient>(
                            this->server->GetServer().Connect(
                                {"/editor/parameters"}),
                            this->charPreset);
                }
            }
            if (this->screen) {
                this->screen->Start(KgrNetConfig::ResponseTimeout);
            }
        } else {
            throw SlokedError("EditorInstance: Already running");
        }
    }

    void SlokedEditorInstance::Stop() {
        if (this->running.load()) {
            std::thread([this] {
                std::unique_lock lock(this->termination_mtx);
                this->closeables.Close();
                this->screen = nullptr;
                this->screenProvider = nullptr;
                this->charPresetUpdater = nullptr;
                this->server = nullptr;
                this->serviceProvider = nullptr;
                this->crypto = nullptr;
                this->running = false;
                this->handles.clear();
                this->termination_cv.notify_all();
            }).detach();
        } else {
            throw SlokedError("EditorInstance: Not running");
        }
    }

    void SlokedEditorInstance::Wait() {
        std::unique_lock lock(this->termination_mtx);
        while (this->running.load()) {
            this->termination_cv.wait(lock);
        }
    }

    void SlokedEditorInstance::Close() {
        this->Stop();
        this->Wait();
    }

    SlokedCharPreset &SlokedEditorInstance::GetCharPreset() {
        return this->charPreset;
    }

    SlokedSchedulerThread &SlokedEditorInstance::GetScheduler() {
        return this->sched;
    }

    SlokedThreadManager &SlokedEditorInstance::GetThreadManager() {
        return this->threadManager;
    }

    SlokedIOPoller &SlokedEditorInstance::GetIO() {
        return *this->ioPoller;
    }

    SlokedNetworkFacade &SlokedEditorInstance::GetNetwork() {
        return this->network;
    }

    bool SlokedEditorInstance::HasCrypto() const {
        return this->crypto != nullptr;
    }

    SlokedCryptoFacade &SlokedEditorInstance::GetCrypto() {
        if (this->crypto) {
            return *this->crypto;
        } else {
            throw SlokedError("EditorInstance: Crypto not defined");
        }
    }

    SlokedServerFacade &SlokedEditorInstance::GetServer() {
        if (this->server) {
            return *this->server;
        } else {
            throw SlokedError("EditorInstance: Server not defined");
        }
    }

    SlokedServiceDependencyProvider &
        SlokedEditorInstance::GetServiceProvider() {
        if (this->serviceProvider) {
            return *this->serviceProvider;
        } else {
            throw SlokedError("EditorInstance: Service provider not defined");
        }
    }

    bool SlokedEditorInstance::HasScreen() const {
        return this->screen != nullptr;
    }

    SlokedScreenServer &SlokedEditorInstance::GetScreen() const {
        if (this->screen) {
            return *this->screen;
        } else {
            throw SlokedError("EditorInstance: Screen not defined");
        }
    }

    KgrContextManager<KgrLocalContext>
        &SlokedEditorInstance::GetContextManager() {
        return this->contextManager.GetManager();
    }
}  // namespace sloked