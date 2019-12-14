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

#ifndef SLOKED_SCRIPT_SCRIPTENGINE_H_
#define SLOKED_SCRIPT_SCRIPTENGINE_H_

#include "sloked/core/Closeable.h"
#include "sloked/kgr/NamedServer.h"

namespace sloked {

    class SlokedScriptEngine : public SlokedCloseable {
     public:
        virtual ~SlokedScriptEngine() = default;
        virtual void Start(const std::string &) = 0;
        virtual void BindServer(const std::string &, KgrNamedServer &) = 0;
    };
}

#endif
