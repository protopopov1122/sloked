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

#ifndef SLOKED_NAMESPACE_ROOT_H_
#define SLOKED_NAMESPACE_ROOT_H_

#include "sloked/namespace/Resolve.h"
#include "sloked/namespace/Mount.h"
#include <memory>

namespace sloked {

    class SlokedRootNamespace {
     public:
        virtual ~SlokedRootNamespace() = default;
        virtual SlokedPathResolver &GetResolver() = 0;
        virtual SlokedMountableNamespace &GetRoot() = 0;
        virtual SlokedNamespaceMounter &GetMounter() = 0;
    };

    class SlokedRootNamespaceFactory {
     public:
        virtual ~SlokedRootNamespaceFactory() = default;
        virtual std::unique_ptr<SlokedRootNamespace> Build() const = 0;
    };
}

#endif