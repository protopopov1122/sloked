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

#include "sloked/namespace/win32/Filesystem.h"

#include "sloked/core/Error.h"
#include "sloked/core/URI.h"
#include "sloked/filesystem/win32/File.h"
#include <cstring>

namespace sloked {

    static const SlokedPath::Preset Win32Preset{"\\/", ".", "..", ""};

    static SlokedPath Win32Path(std::string_view path) {
        if (path.size() >= 2 && std::isalpha(path[0]) && path[1] == ':') {
            return SlokedPath(path.substr(0, 2), path.substr(2), Win32Preset);
        } else {
            return SlokedPath(path, Win32Preset);
        }
    }

    SlokedWin32FilesystemAdapter::SlokedWin32FilesystemAdapter(
        std::string_view root)
        : rootPath(Win32Path(root)) {}
    
    const SlokedPath::Preset &SlokedWin32FilesystemAdapter::GetPreset() {
        return Win32Preset;
    }

    const SlokedPath &SlokedWin32FilesystemAdapter::GetRoot() const {
        return this->rootPath;
    }

    std::unique_ptr<SlokedFile> SlokedWin32FilesystemAdapter::NewFile(
        const SlokedPath &path) const {
        SlokedPath realPath = path.Root();
        if (path.IsAbsolute()) {
            realPath = this->GetRoot().RelativeTo(path.RelativeTo(path.Root().Migrate("")));
        } else {
            realPath = this->GetRoot().RelativeTo(path);
        }
        if (!this->GetRoot().IsParent(realPath)) {
            throw SlokedError(std::string{"Path out of root scope: "} +
                              path.ToString());
        } else {
            return std::make_unique<SlokedWin32File>(realPath.ToString());
        }
    }
    SlokedPath SlokedWin32FilesystemAdapter::ToPath(
        const std::string &path) const {
        return Win32Path(path);
    }

    std::string SlokedWin32FilesystemAdapter::ToURI(
        const SlokedPath &path) const {
        SlokedPath realPath =
            path.IsAbsolute() ? path.RelativeTo(path.Root()) : path;
        return SlokedUri("file", realPath.RelativeTo(this->rootPath).Migrate(SlokedPath::Preset{"/"}).ToString())
            .ToString();
    }

    std::unique_ptr<SlokedFilesystemAdapter>
        SlokedWin32FilesystemAdapter::Rebase(std::string_view path) const {
        return std::make_unique<SlokedWin32FilesystemAdapter>(path);
    }
}  // namespace sloked