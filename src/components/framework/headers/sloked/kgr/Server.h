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

#ifndef SLOKED_KGR_SERVER_H_
#define SLOKED_KGR_SERVER_H_

#include <functional>
#include <memory>

#include "sloked/kgr/Service.h"

namespace sloked {

    using KgrServerServiceId = uint16_t;

    template <typename T>
    class KgrAbstractServer {
     public:
        using Connector = std::function<TaskResult<std::unique_ptr<KgrPipe>>()>;
        virtual ~KgrAbstractServer() = default;
        virtual TaskResult<std::unique_ptr<KgrPipe>> Connect(T) = 0;
        virtual Connector GetConnector(T) = 0;

        virtual TaskResult<void> Register(T, std::unique_ptr<KgrService>) = 0;
        virtual TaskResult<bool> Registered(T) = 0;
        virtual TaskResult<void> Deregister(T) = 0;
    };

    class KgrServer : public KgrAbstractServer<KgrServerServiceId> {
     public:
        using ServiceId = KgrServerServiceId;
        using KgrAbstractServer<KgrServerServiceId>::Register;
        virtual TaskResult<ServiceId> Register(std::unique_ptr<KgrService>) = 0;
    };
}  // namespace sloked

#endif