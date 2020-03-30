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

#ifndef SLOKED_BOOTSTRAP_NAMESPACE_H_
#define SLOKED_BOOTSTRAP_NAMESPACE_H_

#include "sloked/namespace/Root.h"
#include "sloked/namespace/Virtual.h"

namespace sloked {

    class SlokedBootstrapRootNamespace : public SlokedRootNamespace {
     public:
        SlokedBootstrapRootNamespace();
        SlokedPathResolver &GetResolver() final;
        SlokedMountableNamespace &GetRoot() final;
        SlokedNamespaceMounter &GetMounter() final;

     private:
        SlokedDefaultVirtualNamespace root;
        SlokedDefaultNamespaceMounter mounter;
        SlokedPathResolver resolver;
    };

    class SlokedBootstrapRootNamespaceFactory
        : public SlokedRootNamespaceFactory {
     public:
        std::unique_ptr<SlokedRootNamespace> Build() const final;
    };
}  // namespace sloked

#endif