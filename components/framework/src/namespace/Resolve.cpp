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

#include "sloked/namespace/Resolve.h"

namespace sloked {

    SlokedPathResolver::SlokedPathResolver(SlokedPath currentDir, std::optional<SlokedPath> homeDir)
        : currentDir(std::move(currentDir)), homeDir(std::move(homeDir)) {}

    const SlokedPath &SlokedPathResolver::GetCurrentDir() const {
        return this->currentDir;
    }

    void SlokedPathResolver::ChangeDir(SlokedPath dir) {
        this->currentDir = std::move(dir);
    }

    const std::optional<SlokedPath> &SlokedPathResolver::GetHomeDir() const {
        return this->homeDir;
    }

    void SlokedPathResolver::ChangeHomeDir(std::optional<SlokedPath> path) {
        this->homeDir = std::move(path);
    }

    SlokedPath SlokedPathResolver::Resolve(SlokedPath dir) {
        if (dir.IsAbsolute()) {
            if (this->homeDir.has_value() &&
                !dir.Components().empty() &&
                dir.Components().front() == dir.GetPreset().GetHomeDir() &&
                !dir.GetPreset().GetHomeDir().empty()) {
                return dir.RelativeTo(SlokedPath(dir.GetPrefix(), dir.GetPreset().GetHomeDir(), dir.GetPreset())).RelativeTo(this->homeDir.value());
            } else {
                return dir;
            }
        } else {
            return dir.RelativeTo(this->currentDir);
        }
    }
}