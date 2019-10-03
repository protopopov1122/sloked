/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

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

#ifndef SLOKED_FILESYSTEM_FILE_H_
#define SLOKED_FILESYSTEM_FILE_H_

#include "sloked/core/IO.h"
#include "sloked/core/Permission.h"
#include "sloked/filesystem/Permissions.h"
#include <memory>
#include <functional>

namespace sloked {

    class SlokedFile : public SlokedPermissionAuthority<SlokedFilesystemPermission> {
     public:
        using Size = uint64_t;
        using FileVisitor = std::function<void(const std::string &)>;
        enum class Type  {
            RegularFile,
            Directory,
            Unsupported
        };

        virtual ~SlokedFile() = default;

        virtual bool IsFile() const = 0;
        virtual bool IsDirectory() const = 0;
        virtual const std::string &GetPath() const = 0;
        virtual std::string GetName() const = 0;
        virtual std::string GetParent() const = 0;
        virtual bool Exists() const = 0;
        virtual void Delete() const = 0;
        virtual void Rename(const std::string &) const = 0;
        virtual void Create() const = 0;
        virtual void Mkdir() const = 0;

        virtual Size GetSize() const = 0;
        virtual std::unique_ptr<SlokedIOReader> Reader() const = 0;
        virtual std::unique_ptr<SlokedIOWriter> Writer() const = 0;
        virtual std::unique_ptr<SlokedIOView> View() const = 0;

        virtual std::unique_ptr<SlokedFile> GetFile(std::string_view) const = 0;
        virtual void ListFiles(FileVisitor) const = 0;
        virtual void Traverse(FileVisitor, bool = false) const = 0;
    };
}

#endif