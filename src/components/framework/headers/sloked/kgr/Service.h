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

#ifndef SLOKED_KGR_SERVICE_H_
#define SLOKED_KGR_SERVICE_H_

#include <memory>

#include "sloked/kgr/Context.h"
#include "sloked/kgr/Pipe.h"
#include "sloked/sched/Task.h"

namespace sloked {

    class KgrService {
     public:
        virtual ~KgrService() = default;
        virtual TaskResult<void> Attach(std::unique_ptr<KgrPipe>) = 0;
    };
}  // namespace sloked

#endif