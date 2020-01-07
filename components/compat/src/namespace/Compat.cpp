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

#include "sloked/namespace/Compat.h"


#ifdef SLOKED_PLATFORM_POSIX
#include "sloked/namespace/posix/Environment.h"
#include "sloked/namespace/posix/Filesystem.h"

namespace sloked {

    std::unique_ptr<SlokedFilesystemAdapter> SlokedNamespaceCompat::NewFilesystem(const std::string &path) {
        return std::make_unique<SlokedPosixFilesystemAdapter>(path);
    }

    std::unique_ptr<SlokedFilesystemAdapter> SlokedNamespaceCompat::NewRootFilesystem() {
        return std::make_unique<SlokedPosixFilesystemAdapter>("/");
    }

    SlokedPath SlokedNamespaceCompat::GetWorkDir() {
        return SlokedPosixNamespaceEnvironment::WorkDir();
    }

    SlokedPath SlokedNamespaceCompat::GetHomeDir() {
        return SlokedPosixNamespaceEnvironment::HomeDir();
    }
}
#else
#error "Build system error: Platform not defined"
#endif