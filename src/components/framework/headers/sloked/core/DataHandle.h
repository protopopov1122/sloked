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

#ifndef SLOKED_CORE_DATAHANDLE_H_
#define SLOKED_CORE_DATAHANDLE_H_

#include <memory>

#include "sloked/Base.h"

namespace sloked {

    class SlokedDataHandle {
     public:
        virtual ~SlokedDataHandle() = default;
    };

    template <typename T>
    class SlokedTypedDataHandle : public SlokedDataHandle {
     public:
        SlokedTypedDataHandle(std::unique_ptr<T> ptr) : ptr(std::move(ptr)) {}

        virtual ~SlokedTypedDataHandle() = default;

        static std::unique_ptr<SlokedDataHandle> Wrap(std::unique_ptr<T> ptr) {
            return std::make_unique<SlokedTypedDataHandle<T>>(std::move(ptr));
        }

     private:
        std::unique_ptr<T> ptr;
    };
}  // namespace sloked

#endif