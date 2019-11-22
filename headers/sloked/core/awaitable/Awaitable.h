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

#ifndef SLOKED_CORE_AWAITABLE_AWAITABLE_H_
#define SLOKED_CORE_AWAITABLE_AWAITABLE_H_

#include "sloked/Base.h"
#include <cinttypes>
#include <functional>
#include <memory>

namespace sloked {

    class SlokedIOAwaitable {
     public:
        using SystemId = intptr_t;
        virtual ~SlokedIOAwaitable() = default;
        virtual SystemId GetSystemId() const = 0;
    };

    class SlokedIOPoll {
     public:
        virtual ~SlokedIOPoll() = default;
        virtual SlokedIOAwaitable::SystemId GetSystemId() const = 0;
        virtual std::function<void()> Attach(std::unique_ptr<SlokedIOAwaitable>, std::function<void()>) = 0;
        virtual void Await(long = 0) = 0;
    };
}

#endif