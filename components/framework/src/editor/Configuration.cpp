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

#include "sloked/editor/Configuration.h"
#include "sloked/kgr/Serialize.h"
#include "sloked/kgr/Path.h"
#include "sloked/core/Error.h"
#include <fstream>

namespace sloked {

    KgrValue SlokedConfigurationLoader::LoadFile(const std::string &path) {
        KgrJsonSerializer serializer;
        std::ifstream is(path);
        if (is.good()) {
            return serializer.Deserialize(is);
        } else {
            return {};
        }
    }

    SlokedXdgConfigurationLoader::SlokedXdgConfigurationLoader(const std::string &configName)
        : name(configName) {}

    KgrValue SlokedXdgConfigurationLoader::Load() const {
        const char *xdg_config_dir = getenv("XDG_CONFIG_DIR");
        std::string configFile;
        if (xdg_config_dir == nullptr) {
            configFile = getenv("HOME");
            configFile.append("/.config");
        } else {
            configFile = xdg_config_dir;
        }
        configFile.append("/sloked/");
        configFile.append(this->name);
        configFile.append(".json");
        return SlokedConfigurationLoader::LoadFile(configFile);
    }

    SlokedConfiguration::SlokedConfiguration(std::initializer_list<KgrValue> layers)
        : layers(layers) {}

    KgrValue SlokedConfiguration::Find(const std::string &query) const {
        for (const auto &layer : this->layers) {
            auto result = KgrPath::Traverse(layer, SlokedPath{query});
            if (result.has_value()) {
                return result.value();
            }
        }
        throw SlokedError("Configuration: \'" + query + "\' not found");
    }

    bool SlokedConfiguration::Has(const std::string &query) const {
        for (const auto &layer : this->layers) {
            auto result = KgrPath::Traverse(layer, SlokedPath{query});
            if (result.has_value()) {
                return true;
            }
        }
        return false;
    }
}