/*
  SPDX-License-Identifier: LGPL-3.0-or-later

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/namespace/posix/Filesystem.h"
#include "sloked/filesystem/posix/File.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedPosixFilesystemAdapter::SlokedPosixFilesystemAdapter(std::string_view root)
        : rootPath(root) {}

    const SlokedPath &SlokedPosixFilesystemAdapter::GetRoot() const {
        return this->rootPath;
    }

    std::unique_ptr<SlokedFile> SlokedPosixFilesystemAdapter::NewFile(const SlokedPath &path) const {
        SlokedPath realPath = SlokedPath::Root;
        if (path.IsAbsolute()) {
            realPath = this->GetRoot().RelativeTo(path.RelativeTo(SlokedPath::Root));
        } else {
            realPath = this->GetRoot().RelativeTo(path);
        }
        if (!this->GetRoot().IsChildOrSelf(realPath)) {
            throw SlokedError(std::string{"Path out of root scope: "} + path.ToString());
        } else {
            return std::make_unique<SlokedPosixFile>(realPath.ToString());
        }
    }
}