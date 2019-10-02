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

#ifndef SLOKED_SERVICES_SERVICE_H_
#define SLOKED_SERVICES_SERVICE_H_

#include "sloked/kgr/local/Context.h"
#include "sloked/core/Error.h"
#include <queue>
#include <functional>

namespace sloked {

    class SlokedServiceContext : public KgrLocalContext {
     public:
        using KgrLocalContext::KgrLocalContext;
        void Run() override;

     protected:
        void SendResponse(KgrValue &&);
        void OnRequest(std::function<void(const KgrValue &)>);

        virtual void ProcessRequest(const KgrValue &) = 0;
        virtual void HandleError(const SlokedError &);

     private:
        std::queue<std::function<void(const KgrValue &)>> requestHandlers;
    };
}

#endif