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

#ifndef SLOKED_CORE_AWAITABLE_AWAITABLE_H_
#define SLOKED_CORE_AWAITABLE_AWAITABLE_H_

#include <chrono>
#include <cinttypes>
#include <functional>
#include <memory>

#include "sloked/Base.h"

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
        virtual bool Empty() const = 0;
        virtual std::function<void()> Attach(std::unique_ptr<SlokedIOAwaitable>,
                                             std::function<void()>) = 0;
        virtual void Await(std::chrono::system_clock::duration =
                               std::chrono::system_clock::duration::zero()) = 0;
    };
}  // namespace sloked

#endif