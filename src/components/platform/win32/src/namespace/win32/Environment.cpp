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

#include "sloked/namespace/win32/Environment.h"
#include "sloked/namespace/win32/Path.h"
#include <windows.h>

namespace sloked {

    SlokedPath SlokedWin32NamespaceEnvironment::WorkDir() {
        char cwd[MAX_PATH + 1];
        GetCurrentDirectoryA(MAX_PATH, cwd);
        return Win32Path(cwd);
    }

    SlokedPath SlokedWin32NamespaceEnvironment::HomeDir() {
        const char *pw = getenv("USERPROFILE");
        return Win32Path(pw);
    }
}  // namespace sloked