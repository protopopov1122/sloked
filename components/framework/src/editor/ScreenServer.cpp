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

#include "sloked/editor/ScreenServer.h"
#include "sloked/services/Screen.h"
#include "sloked/services/ScreenInput.h"
#include "sloked/services/ScreenSize.h"
#include "sloked/services/TextPane.h"
#include "sloked/kgr/net/Config.h"

namespace sloked {

    SlokedScreenServer::SlokedScreenServer(KgrNamedServer &server, SlokedScreenProvider &provider, SlokedScreenSize &screenSize, KgrContextManager<KgrLocalContext> &contextManager)
        : server(server), provider(provider), size(screenSize), work{false} {
        this->server.Register("screen::manager", std::make_unique<SlokedScreenService>(this->provider.GetScreen(),
            this->provider.GetEncoding(), this->server.GetConnector("document::cursor"), this->server.GetConnector("document::notify"), contextManager));
        this->server.Register("screen::size.notify", std::make_unique<SlokedScreenSizeNotificationService>(screenSize, contextManager));
        this->server.Register("screen::component::input.notify", std::make_unique<SlokedScreenInputNotificationService>(this->provider.GetScreen(), this->provider.GetEncoding(), contextManager));
        this->server.Register("screen::component::input.forward", std::make_unique<SlokedScreenInputForwardingService>(this->provider.GetScreen(), this->provider.GetEncoding(), contextManager));
        this->server.Register("screen::component::text.pane", std::make_unique<SlokedTextPaneService>(this->provider.GetScreen(), this->provider.GetEncoding(), contextManager));
    }

    SlokedScreenServer::~SlokedScreenServer() {
        this->Close();
    }

    void SlokedScreenServer::Redraw() {
        this->renderRequested = true;
    }

    bool SlokedScreenServer::IsRunning() const {
        return this->work.load();
    }

    void SlokedScreenServer::Start(std::chrono::system_clock::duration timeout) {
        if (!this->work.exchange(true)) {
            this->worker = std::thread([this, timeout] {
                this->Run(timeout);
            });
        }
    }

    void SlokedScreenServer::Close() {
        if (this->work.exchange(false) && this->worker.joinable()) {
            this->worker.join();
        }
    }

    SlokedScreenProvider &SlokedScreenServer::GetScreen() const {
        return this->provider;
    }

    SlokedScreenSize &SlokedScreenServer::GetScreenSize() const {
        return this->size;
    }

    void SlokedScreenServer::Run(std::chrono::system_clock::duration timeout) {
        this->provider.GetScreen().Lock([this](auto &screen) {
            screen.OnUpdate([this] {
                this->renderRequested = true;
            });
        });
        
        while (work.load()) {
            if (this->renderRequested.load()) {
                this->renderRequested = false;
                this->provider.Render([&](auto &screen) {
                    screen.UpdateDimensions();
                    screen.Render();
                });
            }

            auto input = this->provider.ReceiveInput(timeout);
            for (const auto &evt : input) {
                this->provider.GetScreen().Lock([&](auto &screen) {
                    screen.ProcessInput(evt);
                });
            }
        }
        this->provider.GetScreen().Lock([](auto &screen) {
            screen.OnUpdate(nullptr);
        });
    }

    SlokedScreenServerContainer::Instance::Instance(KgrNamedServer &server, std::unique_ptr<SlokedScreenBasis> basis, KgrContextManager<KgrLocalContext> &contextManager)
        : server(server, basis->GetProvider(), basis->GetSize(), contextManager), basis(std::move(basis)) {}
    
    SlokedScreenServerContainer::SlokedScreenServerContainer() {
        this->contextManager.Start();
    }

    SlokedScreenServer &SlokedScreenServerContainer::Spawn(const std::string &id, KgrNamedServer &server, std::unique_ptr<SlokedScreenBasis> basis) {
        if (this->screens.count(id) == 0) {
            auto instance = std::make_unique<Instance>(server, std::move(basis), this->contextManager.GetManager());
            this->screens.emplace(id, std::move(instance));
            this->screens.at(id)->server.Start(KgrNetConfig::ResponseTimeout);
            return this->screens.at(id)->server;
        } else {
            throw SlokedError("ScreenServerContainer: Server \'" + id + "\' already registered");
        }
    }

    bool SlokedScreenServerContainer::Has(const std::string &id) const {
        return this->screens.count(id) != 0;
    }

    SlokedScreenServer &SlokedScreenServerContainer::Get(const std::string &id) const {
        if (this->Has(id)) {
            return this->screens.at(id)->server;
        } else {
            throw SlokedError("ScreenServerContainer: Server \'" + id + "\' not defined");
        }
    }

    void SlokedScreenServerContainer::Shutdown(const std::string &id) {
        if (this->Has(id)) {
            this->screens.at(id)->server.Close();
            this->screens.erase(id);
        } else {
            throw SlokedError("ScreenServerContainer: Server \'" + id + "\' not defined");       
        }
    }

    void SlokedScreenServerContainer::Close() {
        for (auto &instance : this->screens) {
            instance.second->server.Close();
        }
        this->contextManager.Close();
        this->screens.clear();
    }
}