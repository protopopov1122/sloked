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

#ifndef SLOKED_SERVICES_SHUTDOWN_H_
#define SLOKED_SERVICES_SHUTDOWN_H_

#include "sloked/kgr/Service.h"
#include "sloked/editor/Shutdown.h"
#include "sloked/kgr/local/Context.h"

namespace sloked {

    class SlokedShutdownService : public KgrService {
     public:
        SlokedShutdownService(SlokedEditorShutdown &,
                                    KgrContextManager<KgrLocalContext> &);
        TaskResult<void> Attach(std::unique_ptr<KgrPipe>) override;

     private:
        SlokedEditorShutdown &shutdown;
        KgrContextManager<KgrLocalContext> &contextManager;
    };

    class SlokedShutdownClient {
     public:
        SlokedShutdownClient(std::unique_ptr<KgrPipe>);
        void RequestShutdown();

     private:
        KgrAsyncPipe pipe;
    };
}

#endif