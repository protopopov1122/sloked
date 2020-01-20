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

#include "sloked/editor/PosixConfiguration.h"

namespace sloked {

    KgrValue SlokedXdgConfigurationLoader::Load(const std::string &name) const {
        const char *xdg_config_dir = getenv("XDG_CONFIG_DIR");
        std::string configFile;
        if (xdg_config_dir == nullptr) {
            configFile = getenv("HOME");
            configFile.append("/.config");
        } else {
            configFile = xdg_config_dir;
        }
        configFile.append("/sloked/");
        configFile.append(name);
        configFile.append(".json");
        return SlokedConfigurationLoader::LoadFile(configFile);
    }
}