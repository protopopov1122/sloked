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

#include "sloked/editor/ScreenServer.h"
#include "sloked/services/Screen.h"
#include "sloked/services/ScreenInput.h"
#include "sloked/services/TextPane.h"

namespace sloked {

    SlokedScreenServer::SlokedScreenServer(KgrNamedServer &server, SlokedScreenProvider &provider, KgrContextManager<KgrLocalContext> &contextManager)
        : server(server), provider(provider), work{false} {
        this->server.Register("screen::manager", std::make_unique<SlokedScreenService>(this->provider.GetScreen(),
            this->provider.GetEncoding(), this->server.GetConnector("document::cursor"), this->server.GetConnector("document::notify"), contextManager));
        this->server.Register("screen::component::input.notify", std::make_unique<SlokedScreenInputNotificationService>(this->provider.GetScreen(), this->provider.GetEncoding(), contextManager));
        this->server.Register("screen::component::input.forward", std::make_unique<SlokedScreenInputForwardingService>(this->provider.GetScreen(), this->provider.GetEncoding(), contextManager));
        this->server.Register("screen::component::text.pane", std::make_unique<SlokedTextPaneService>(this->provider.GetScreen(), this->provider.GetEncoding(), contextManager));
    }

    SlokedScreenServer::~SlokedScreenServer() {
        this->Close();
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
        this->provider.GetScreen().Lock([this](auto &screen) {
            screen.OnUpdate(nullptr);
        });
    }
}