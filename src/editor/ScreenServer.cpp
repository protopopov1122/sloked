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

namespace sloked {

    SlokedScreenServer::SlokedScreenServer(KgrNamedServer &server, SlokedScreenProvider &provider)
        : server(server), provider(provider) {}

    void SlokedScreenServer::Run(std::chrono::system_clock::duration timeout) {
        if (!this->work.exchange(true)) {
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

    void SlokedScreenServer::Close() {
        this->work = false;
    }
}