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

#include "sloked/compat/editor/configuration/Compat.h"

#ifdef SLOKED_PLATFORM_POSIX
#include "sloked/editor/PosixConfiguration.h"

namespace sloked {

    SlokedConfigurationLoader &SlokedConfigurationLoaderCompat::GetLoader() {
        static SlokedXdgConfigurationLoader loader;
        return loader;
    }
}  // namespace sloked

#elif defined(SLOKED_PLATFORM_WIN32)
#include "sloked/editor/Win32Configuration.h"

namespace sloked {

    SlokedConfigurationLoader &SlokedConfigurationLoaderCompat::GetLoader() {
        static SlokedWin32ConfigurationLoader loader;
        return loader;
    }
}  // namespace sloked

#else
#error "Build system error: Platform not defined"
#endif