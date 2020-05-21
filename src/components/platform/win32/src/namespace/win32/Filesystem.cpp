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
#include "sloked/core/String.h"
#include "sloked/namespace/win32/Path.h"
#include "sloked/filesystem/win32/File.h"
#include <cstring>

namespace sloked {

    SlokedPath Win32Path(std::string_view rawpath) {
        SlokedPath path{""};
        if (rawpath.size() >= 2 && std::isalpha(rawpath[0]) && rawpath[1] == ':') {
            std::string drive{"$Drive"};
            drive += rawpath.substr(0, 1);
            path = {drive};
            rawpath.remove_prefix(2);
        }
        std::string_view::size_type lastDelim{0};
        for (auto it = rawpath.find("\\"); it != rawpath.npos; it = rawpath.find("\\", lastDelim)) {
            if (it == 0) {
                path = path.RelativeTo(path.Root());
            }
            if (it - lastDelim > 0) {
                path = path.Child(rawpath.substr(lastDelim, it - lastDelim));
            }
            lastDelim = it + 1;
        }
        if (lastDelim < rawpath.size()) {
            path = path.Child(rawpath.substr(lastDelim));
        }
        return path;
    }

    std::string PathToWin32(const SlokedPath &path) {
        std::string rawpath{};
        if (path.IsAbsolute()) {
            rawpath.push_back('\\');
        }
        for (auto it = path.Components().begin(); it != path.Components().end(); ++it) {
            if (it == path.Components().begin() &&
                starts_with(*it, "$Drive")) {
                rawpath = it->substr(6) + ":" + rawpath;
            } else {
                rawpath.append(*it);
                if (it + 1 != path.Components().end()) {
                    rawpath.push_back('\\');
                }
            }
        }
        return rawpath;
    }

    SlokedWin32FilesystemAdapter::SlokedWin32FilesystemAdapter(
        std::string_view root)
        : rootPath(Win32Path(root)) {}

    const SlokedPath &SlokedWin32FilesystemAdapter::GetRoot() const {
        return this->rootPath;
    }

    std::unique_ptr<SlokedFile> SlokedWin32FilesystemAdapter::NewFile(
        const SlokedPath &path) const {
        SlokedPath realPath = path.Root();
        if (path.IsAbsolute()) {
            const auto relPath = path.RelativeTo(path.Root());
            realPath = this->GetRoot().RelativeTo(relPath);
        } else {
            realPath = this->GetRoot().RelativeTo(path);
        }
        if (!this->GetRoot().IsParent(realPath)) {
            throw SlokedError(std::string{"Path out of root scope: "} +
                              path.ToString());
        } else {
            return std::make_unique<SlokedWin32File>(PathToWin32(realPath));
        }
    }
    SlokedPath SlokedWin32FilesystemAdapter::ToPath(
        const std::string &path) const {
        return Win32Path(path);
    }
    
    std::string SlokedWin32FilesystemAdapter::FromPath(const SlokedPath &path) const {
        return PathToWin32(path);
    }

    std::string SlokedWin32FilesystemAdapter::ToURI(
        const SlokedPath &path) const {
        SlokedPath realPath =
            path.IsAbsolute() ? path.RelativeTo(path.Root()) : path;
        return SlokedUri("file", realPath.RelativeTo(this->rootPath).ToString())
            .ToString();
    }

    std::unique_ptr<SlokedFilesystemAdapter>
        SlokedWin32FilesystemAdapter::Rebase(std::string_view path) const {
        return std::make_unique<SlokedWin32FilesystemAdapter>(path);
    }
}  // namespace sloked