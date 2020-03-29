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

#include "sloked/frontend/Namespace.h"

#include "sloked/namespace/Compat.h"
#include "sloked/namespace/Empty.h"

namespace sloked {

    SlokedFrontendRootNamespace::SlokedFrontendRootNamespace()
        : root(std::make_unique<SlokedEmptyNamespace>()),
          mounter(SlokedNamespaceCompat::NewRootFilesystem(), root),
          resolver(SlokedNamespaceCompat::GetWorkDir(),
                   SlokedNamespaceCompat::GetHomeDir()) {}

    SlokedPathResolver &SlokedFrontendRootNamespace::GetResolver() {
        return this->resolver;
    }

    SlokedMountableNamespace &SlokedFrontendRootNamespace::GetRoot() {
        return this->root;
    }

    SlokedNamespaceMounter &SlokedFrontendRootNamespace::GetMounter() {
        return this->mounter;
    }

    std::unique_ptr<SlokedRootNamespace>
        SlokedFrontendRootNamespaceFactory::Build() const {
        return std::make_unique<SlokedFrontendRootNamespace>();
    }
}  // namespace sloked