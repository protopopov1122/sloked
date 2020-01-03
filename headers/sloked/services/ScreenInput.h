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

#ifndef SLOKED_SERVICES_SCREENINPUT_H_
#define SLOKED_SERVICES_SCREENINPUT_H_

#include "sloked/services/Service.h"
#include "sloked/kgr/Service.h"
#include "sloked/kgr/Server.h"
#include "sloked/core/Monitor.h"
#include "sloked/screen/Component.h"
#include "sloked/core/Encoding.h"

namespace sloked {

    class SlokedScreenInputNotificationService : public KgrService {
     public:
        SlokedScreenInputNotificationService(SlokedMonitor<SlokedScreenComponent &> &, const Encoding &, KgrContextManager<KgrLocalContext> &);
        void Attach(std::unique_ptr<KgrPipe>) override;
    
     private:
        SlokedMonitor<SlokedScreenComponent &> &root;
        const Encoding &encoding;
        KgrContextManager<KgrLocalContext> &contextManager;
    };
    
    class SlokedScreenInputNotificationClient {
     public:
        using Callback = std::function<void(const SlokedKeyboardInput &)>;
        SlokedScreenInputNotificationClient(std::unique_ptr<KgrPipe>, const Encoding &, std::function<bool()> = nullptr);
        void Listen(const std::string &, bool, const std::vector<std::pair<SlokedControlKey, bool>> &, Callback, bool = false);
        void Listen(const std::string &, Callback, bool = false);
        void Close();

     private:
        std::unique_ptr<KgrPipe> pipe;
        const Encoding &encoding;
        std::function<bool()> holdsLock;
    };

    class SlokedScreenInputForwardingService : public KgrService {
     public:
        SlokedScreenInputForwardingService(SlokedMonitor<SlokedScreenComponent &> &, const Encoding &, KgrContextManager<KgrLocalContext> &);
        void Attach(std::unique_ptr<KgrPipe>) override;
    
     private:
        SlokedMonitor<SlokedScreenComponent &> &root;
        const Encoding &encoding;
        KgrContextManager<KgrLocalContext> &contextManager;
    };
    
    class SlokedScreenInputForwardingClient {
     public:
        SlokedScreenInputForwardingClient(std::unique_ptr<KgrPipe>, const Encoding &, std::function<bool()> = nullptr);
        void Send(const std::string &, const SlokedKeyboardInput &);

     private:
        SlokedServiceClient client;
        const Encoding &encoding;
        std::function<bool()> holdsLock;
    };
}

#endif