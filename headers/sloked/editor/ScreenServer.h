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

#ifndef SLOKED_EDITOR_SCREENSERVER_H_
#define SLOKED_EDITOR_SCREENSERVER_H_

#include "sloked/core/Closeable.h"
#include "sloked/core/Monitor.h"
#include "sloked/kgr/NamedServer.h"
#include "sloked/screen/Component.h"
#include <atomic>
#include <chrono>

namespace sloked {

    class SlokedScreenProvider {
     public:
        virtual ~SlokedScreenProvider() = default;
        virtual void Render(std::function<void(SlokedScreenComponent &)>) = 0;
        virtual std::vector<SlokedKeyboardInput> ReceiveInput(std::chrono::system_clock::duration) = 0;
        virtual SlokedMonitor<SlokedScreenComponent &> &GetScreen() = 0;
    };

    class SlokedScreenServer : public SlokedCloseable {
     public:
        using InputProcessor = std::function<std::vector<SlokedKeyboardInput>(std::vector<SlokedKeyboardInput>)>;
        SlokedScreenServer(KgrNamedServer &, SlokedScreenProvider &);
        void Run(std::chrono::system_clock::duration);
        void Close() final;

        template <typename A, typename ... B>
        void Register(const std::string &id, std::unique_ptr<A> service, B &&... services) {
            this->server.Register(id, std::move(service));
            if constexpr (sizeof...(B) > 0) {
                this->Register(std::forward<B>(services)...);
            }
        }

     private:
        KgrNamedServer &server;
        SlokedScreenProvider &provider;
        std::atomic<bool> work;
        std::atomic<bool> renderRequested;
    };
}

#endif