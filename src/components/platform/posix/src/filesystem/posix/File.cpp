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

#include "sloked/filesystem/posix/File.h"
#include "sloked/core/Error.h"

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

#include "sloked/filesystem/posix/Reader.h"
#include "sloked/filesystem/posix/View.h"
#include "sloked/filesystem/posix/Writer.h"

namespace sloked {

    static constexpr auto MODE = 0755;

    class DirectoryHandle {
     public:
        DirectoryHandle(const std::string &path) : dir(opendir(path.c_str())) {}

        ~DirectoryHandle() {
            closedir(this->dir);
        }

        dirent *Read() const {
            if (this->dir) {
                return readdir(this->dir);
            } else {
                return nullptr;
            }
        }

     private:
        DIR *dir;
    };

    SlokedPosixFile::SlokedPosixFile(const std::string &path) : path(path) {}

    bool SlokedPosixFile::IsFile() const {
        struct stat stats;
        return stat(this->path.c_str(), &stats) == 0 && S_ISREG(stats.st_mode);
    }

    bool SlokedPosixFile::IsDirectory() const {
        struct stat stats;
        return stat(this->path.c_str(), &stats) == 0 && S_ISDIR(stats.st_mode);
    }

    bool SlokedPosixFile::HasPermission(
        SlokedFilesystemPermission permission) const {
        std::unique_ptr<char[]> pathClone(new char[this->path.size() + 1]);
        strcpy(pathClone.get(), this->path.c_str());
        switch (permission) {
            case SlokedFilesystemPermission::Read:
                return access(this->path.c_str(), R_OK) == 0;

            case SlokedFilesystemPermission::Write:
                return access(this->path.c_str(), W_OK) == 0;

            default:
                return false;
        }
    }

    const std::string &SlokedPosixFile::GetPath() const {
        return this->path;
    }

    std::string SlokedPosixFile::GetName() const {
        std::unique_ptr<char[]> buffer(new char[this->path.size() + 1]);
        strcpy(buffer.get(), this->path.c_str());
        return std::string{basename(buffer.get())};
    }

    uint64_t SlokedPosixFile::GetSize() const {
        struct stat stats;
        if (!stat(this->path.c_str(), &stats)) {
            return stats.st_size;
        } else {
            return 0;
        }
    }

    bool SlokedPosixFile::Exists() const {
        struct stat stats;
        return stat(this->path.c_str(), &stats) == 0;
    }

    std::string SlokedPosixFile::Parent() const {
        std::unique_ptr<char[]> buffer(new char[this->path.size() + 1]);
        strcpy(buffer.get(), this->path.c_str());
        return std::string{dirname(buffer.get())};
    }

    static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag,
                         struct FTW *ftwbuf) {
        return remove(fpath);
    }

    void SlokedPosixFile::Delete() const {
        if (this->IsFile()) {
            if (remove(this->path.c_str()) != 0) {
                throw SlokedError("PosixFile: Error deleting file");
            }
        } else {
            if (nftw(this->path.c_str(), unlink_cb, 64, FTW_DEPTH | FTW_PHYS) != 0) {
                throw SlokedError("PosixFile: Error deleting directory");
            }
        }
    }

    void SlokedPosixFile::Rename(const std::string &name) const {
        if (rename(this->path.c_str(), name.c_str()) != 0) {
            throw SlokedError("PosixFile: Error renaming file");
        }
    }

    void SlokedPosixFile::Create() const {
        if (!this->Exists()) {
            const auto res = creat(this->path.c_str(), MODE);
            if (res > -1) {
                close(res);
            } else {
                throw SlokedError("PosixFile: Error creating file");
            }
        }
    }

    void SlokedPosixFile::Mkdir() const {
        if (!this->Exists()) {
            if (mkdir(this->path.c_str(), MODE) != 0) {
                throw SlokedError("PosixFile: Error creating directory");
            }
        }
    }

    std::unique_ptr<SlokedIOReader> SlokedPosixFile::Reader() const {
        FILE *fp = fopen(this->path.c_str(), "r");
        if (fp) {
            return std::make_unique<SlokedPosixFileReader>(fp);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedIOWriter> SlokedPosixFile::Writer() const {
        FILE *fp = fopen(this->path.c_str(), "w+b");
        if (fp) {
            return std::make_unique<SlokedPosixFileWriter>(fp);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedIOView> SlokedPosixFile::View() const {
        FILE *fp = fopen(this->path.c_str(), "r");
        if (fp) {
            return std::make_unique<SlokedPosixFileView>(fp);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedFile> SlokedPosixFile::GetFile(
        std::string_view name) const {
        std::string path{this->path};
        path.push_back('/');
        path.append(name);
        return std::make_unique<SlokedPosixFile>(path);
    }

    void SlokedPosixFile::ListFiles(FileVisitor visitor) const {
        DirectoryHandle directory(this->path);
        for (dirent *entry = directory.Read(); entry != nullptr;
             entry = directory.Read()) {
            std::string str(entry->d_name);
            if (str != ".." && str != ".") {
                visitor(str);
            }
        }
    }

    static void TraverseDir(const std::string &dir,
                            const SlokedFile::FileVisitor &visitor,
                            bool includes_dirs) {
        DirectoryHandle directory(dir);
        for (dirent *entry = directory.Read(); entry != nullptr;
             entry = directory.Read()) {
            std::string path(entry->d_name);
            if (path != ".." && path != ".") {
                struct stat stats;
                auto fullPath = dir + "/" + path;
                bool is_directory = stat(fullPath.c_str(), &stats) == 0 &&
                                    S_ISDIR(stats.st_mode);
                if (!is_directory || includes_dirs) {
                    visitor(fullPath);
                }
                if (is_directory) {
                    TraverseDir(fullPath, visitor, includes_dirs);
                }
            }
        }
    }

    void SlokedPosixFile::Traverse(FileVisitor visitor,
                                   bool include_dirs) const {
        if (this->IsFile()) {
            visitor(this->path);
        } else {
            TraverseDir(this->path, visitor, include_dirs);
        }
    }
}  // namespace sloked