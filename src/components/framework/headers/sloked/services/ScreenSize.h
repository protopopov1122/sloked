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

#ifndef SLOKED_SERVICES_SCREENSIZE_H_
#define SLOKED_SERVICES_SCREENSIZE_H_

#include <atomic>

#include "sloked/kgr/Server.h"
#include "sloked/kgr/Service.h"
#include "sloked/screen/Size.h"
#include "sloked/services/Service.h"

namespace sloked {

    class SlokedScreenSizeNotificationService : public KgrService {
     public:
        SlokedScreenSizeNotificationService(
            SlokedScreenSize &, KgrContextManager<KgrLocalContext> &);
        TaskResult<void> Attach(std::unique_ptr<KgrPipe>) override;

     private:
        SlokedScreenSize &size;
        KgrContextManager<KgrLocalContext> &contextManager;
    };

    class SlokedScreenSizeNotificationClient {
     public:
        using Callback = std::function<void(const TextPosition &)>;
        SlokedScreenSizeNotificationClient();
        ~SlokedScreenSizeNotificationClient();

        TaskResult<void> Connect(std::unique_ptr<KgrPipe>);
        TextPosition GetSize() const;
        void Listen(Callback);
        void Close();

     private:
        KgrAsyncPipe pipe;
        std::shared_ptr<SlokedStandardLifetime> lifetime;
        std::atomic<TextPosition> currentSize;
    };
}  // namespace sloked

#endif