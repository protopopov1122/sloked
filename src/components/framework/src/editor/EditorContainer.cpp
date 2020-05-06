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

#include "sloked/editor/EditorContainer.h"

#include "sloked/core/Error.h"
#include "sloked/kgr/net/Config.h"
#include "sloked/net/CryptoSocket.h"

namespace sloked {

    SlokedSharedContainerEnvironment::SlokedSharedContainerEnvironment(
        std::unique_ptr<SlokedIOPoll> ioPoll)
        : ioPoll(std::move(ioPoll)), ioPoller(*this->ioPoll, executor),
          scheduler(executor) {}

    void SlokedSharedContainerEnvironment::Start() {
        this->scheduler.Start();
        this->ioPoller.Start(KgrNetConfig::RequestTimeout);
        this->executor.Start();
    }

    void SlokedSharedContainerEnvironment::Close() {
        this->executor.Close();
        this->ioPoller.Close();
        this->threadManager.Close();
        this->scheduler.Close();
    }

    SlokedIOPoller &SlokedSharedContainerEnvironment::GetIO() {
        return this->ioPoller;
    }

    SlokedScheduler &SlokedSharedContainerEnvironment::GetScheduler() {
        return this->scheduler;
    }

    SlokedExecutor &SlokedSharedContainerEnvironment::GetExecutor() {
        return this->executor;
    }

    SlokedExecutor &SlokedSharedContainerEnvironment::GetThreadedExecutor() {
        return this->threadManager;
    }

    SlokedEditorContainer::SlokedEditorContainer(
        SlokedSharedContainerEnvironment &sharedState, SlokedSocketFactory &network)
        : sharedState(sharedState), running{false}, network(network),
          contextManager(sharedState.GetExecutor()) {
        this->closeables.Attach(this->contextManager);
    }

    SlokedCryptoFacade &SlokedEditorContainer::InitializeCrypto(
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

    SlokedServerFacade &SlokedEditorContainer::InitializeServer() {
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

    SlokedServerFacade &SlokedEditorContainer::InitializeServer(
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
                    std::move(socket), this->sharedState.GetIO(),
                    this->sharedState.GetScheduler(), auth));
            this->closeables.Attach(*this->server);
            return *this->server;
        }
    }

    SlokedServiceDependencyProvider &
        SlokedEditorContainer::InitializeServiceProvider(
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

    SlokedScreenServer &SlokedEditorContainer::InitializeScreen(
        SlokedScreenProviderFactory &providers, const SlokedUri &uri, std::unique_ptr<SlokedCharPreset> charPreset) {
        if (this->running.load()) {
            throw SlokedError("EditorInstance: Already running");
        } else if (this->screen != nullptr) {
            throw SlokedError("EditorInstance: Screen already initialized");
        } else {
            this->screenProvider = providers.Make(uri, std::move(charPreset));
            this->screen = std::make_unique<SlokedScreenServer>(
                this->GetServer().GetServer(), *this->screenProvider,
                this->GetContextManager());
            this->closeables.Attach(*this->screen);
            return *this->screen;
        }
    }

    void SlokedEditorContainer::Attach(SlokedCloseable &closeable) {
        this->closeables.Attach(closeable);
    }

    void SlokedEditorContainer::Attach(
        std::unique_ptr<SlokedDataHandle> handle) {
        this->handles.emplace_back(std::move(handle));
    }

    bool SlokedEditorContainer::IsRunning() const {
        return this->running.load();
    }

    void SlokedEditorContainer::Start() {
        if (!this->running.exchange(true)) {
            this->contextManager.Start();
            if (this->server) {
                this->server->Start();
            }
            if (this->screen) {
                this->screen->Start(KgrNetConfig::ResponseTimeout);
            }
        } else {
            throw SlokedError("EditorInstance: Already running");
        }
    }

    void SlokedEditorContainer::Stop() {
        if (this->running.load()) {
            std::thread([this] {
                std::unique_lock lock(this->termination_mtx);
                this->closeables.Close();
                this->screen = nullptr;
                this->screenProvider = nullptr;
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

    void SlokedEditorContainer::Wait() {
        std::unique_lock lock(this->termination_mtx);
        while (this->running.load()) {
            this->termination_cv.wait(lock);
        }
    }

    void SlokedEditorContainer::Close() {
        this->Stop();
        this->Wait();
    }

    SlokedScheduler &SlokedEditorContainer::GetScheduler() {
        return this->sharedState.GetScheduler();
    }

    SlokedExecutor &SlokedEditorContainer::GetExecutor() {
        return this->sharedState.GetExecutor();
    }

    SlokedExecutor &SlokedEditorContainer::GetThreadedExecutor() {
        return this->sharedState.GetThreadedExecutor();
    }

    SlokedIOPoller &SlokedEditorContainer::GetIO() {
        return this->sharedState.GetIO();
    }

    SlokedNetworkFacade &SlokedEditorContainer::GetNetwork() {
        return this->network;
    }

    bool SlokedEditorContainer::HasCrypto() const {
        return this->crypto != nullptr;
    }

    SlokedCryptoFacade &SlokedEditorContainer::GetCrypto() {
        if (this->crypto) {
            return *this->crypto;
        } else {
            throw SlokedError("EditorInstance: Crypto not defined");
        }
    }

    SlokedServerFacade &SlokedEditorContainer::GetServer() {
        if (this->server) {
            return *this->server;
        } else {
            throw SlokedError("EditorInstance: Server not defined");
        }
    }

    SlokedServiceDependencyProvider &
        SlokedEditorContainer::GetServiceProvider() {
        if (this->serviceProvider) {
            return *this->serviceProvider;
        } else {
            throw SlokedError("EditorInstance: Service provider not defined");
        }
    }

    bool SlokedEditorContainer::HasScreen() const {
        return this->screen != nullptr;
    }

    SlokedScreenServer &SlokedEditorContainer::GetScreen() const {
        if (this->screen) {
            return *this->screen;
        } else {
            throw SlokedError("EditorInstance: Screen not defined");
        }
    }

    KgrContextManager<KgrLocalContext>
        &SlokedEditorContainer::GetContextManager() {
        return this->contextManager.GetManager();
    }
}  // namespace sloked