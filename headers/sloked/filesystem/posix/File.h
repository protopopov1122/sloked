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

#ifndef SLOKED_FILESYSTEN_POSIX_FILE_H_
#define SLOKED_FILESYSTEN_POSIX_FILE_H_

#include "sloked/filesystem/File.h"

namespace sloked {

    class SlokedPosixFile : public SlokedFile {
     public:
        SlokedPosixFile(const std::string &);

        bool IsFile() const override;
        bool IsDirectory() const override;
        bool HasPermission(SlokedFilesystemPermission) const override;
        const std::string &GetPath() const override;
        std::string GetName() const override;
        std::string GetParent() const override;
        bool Exists() const override;
        void Delete() const override;
        void Rename(const std::string &) const override;
        void Create() const override;
        void Mkdir() const override;

        uint64_t GetSize() const override;
        std::unique_ptr<SlokedIOReader> Reader() const override;
        std::unique_ptr<SlokedIOWriter> Writer() const override;
        std::unique_ptr<SlokedIOView> View() const override;

        std::unique_ptr<SlokedFile> GetFile(std::string_view) const override;
        void ListFiles(FileVisitor) const override;

     private:
        std::string path;
    };
}

#endif