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

#include "sloked/filesystem/win32/File.h"
#include "sloked/core/Error.h"

#include <windows.h>
#include <shlwapi.h>
#include <io.h>
#include <fileapi.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstring>

#include "sloked/filesystem/win32/Reader.h"
#include "sloked/filesystem/win32/View.h"
#include "sloked/filesystem/win32/Writer.h"

namespace sloked {

    SlokedWin32File::SlokedWin32File(const std::string &path) : path(path) {}

    bool SlokedWin32File::IsFile() const {
		DWORD dwAttrib = GetFileAttributes(TEXT(this->path.c_str()));
		return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
			(dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0);
    }

    bool SlokedWin32File::IsDirectory() const {
		DWORD dwAttrib = GetFileAttributes(TEXT(this->path.c_str()));
		return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
			(dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
    }

    bool SlokedWin32File::HasPermission(
        SlokedFilesystemPermission permission) const {
			
        switch (permission) {
            case SlokedFilesystemPermission::Read:
                return _access(this->path.c_str(), R_OK) == 04;

            case SlokedFilesystemPermission::Write:
                return _access(this->path.c_str(), W_OK) == 02;

            default:
                return false;
        }
    }

    const std::string &SlokedWin32File::GetPath() const {
        return this->path;
    }

    std::string SlokedWin32File::GetName() const {
        std::unique_ptr<char[]> buffer(new char[this->path.size() + 1]);
        strcpy(buffer.get(), this->path.c_str());
		PathStripPathA(buffer.get());
        return std::string{buffer.get()};
    }

    uint64_t SlokedWin32File::GetSize() const {
        struct __stat64 stats;
        if (!_stat64(this->path.c_str(), &stats)) {
            return stats.st_size;
        } else {
            return 0;
        }
    }

    bool SlokedWin32File::Exists() const {
		DWORD dwAttrib = GetFileAttributes(TEXT(this->path.c_str()));
		return dwAttrib != INVALID_FILE_ATTRIBUTES;
    }

    std::string SlokedWin32File::Parent() const {
        std::unique_ptr<char[]> buffer(new char[this->path.size() + 1]);
        strcpy(buffer.get(), this->path.c_str());
		PathRemoveFileSpecA(buffer.get());
        return std::string{buffer.get()};
    }

    void SlokedWin32File::Delete() const {
        if (this->IsFile()) {
            if (DeleteFileA(this->path.c_str()) == 0) {
                throw SlokedError("Win32File: Error deleting file");
            }
        } else {
            if (RemoveDirectoryA(this->path.c_str()) == 0) {
                throw SlokedError("Win32File: Error deleting directory");
            }
        }
    }

    void SlokedWin32File::Rename(const std::string &name) const {
        if (MoveFile(this->path.c_str(), name.c_str()) == 0) {
            throw SlokedError("PosixFile: Error renaming file");
        }
    }

    void SlokedWin32File::Create() const {
        if (!this->Exists()) {
            const auto res = CreateFileA(
			   this->path.c_str(),
			   GENERIC_WRITE, 
			   0,
			   NULL,
			   CREATE_NEW,
			   FILE_ATTRIBUTE_NORMAL,
			   NULL);
            if (res != INVALID_HANDLE_VALUE) {
                CloseHandle(res);
            } else {
                throw SlokedError("PosixFile: Error creating file");
            }
        }
    }

    void SlokedWin32File::Mkdir() const {
        if (!this->Exists()) {
            if (CreateDirectoryA(this->path.c_str(), 0) != 0) {
                throw SlokedError("PosixFile: Error creating directory");
            }
        }
    }

    std::unique_ptr<SlokedIOReader> SlokedWin32File::Reader() const {
        FILE *fp = fopen(this->path.c_str(), "r");
        if (fp) {
            return std::make_unique<SlokedWin32FileReader>(fp);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedIOWriter> SlokedWin32File::Writer() const {
        FILE *fp = fopen(this->path.c_str(), "w+b");
        if (fp) {
            return std::make_unique<SlokedWin32FileWriter>(fp);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedIOView> SlokedWin32File::View() const {
        FILE *fp = fopen(this->path.c_str(), "r");
        if (fp) {
            return std::make_unique<SlokedWin32FileView>(fp);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedFile> SlokedWin32File::GetFile(
        std::string_view name) const {
        std::string path{this->path};
        path.push_back('\\');
        path.append(name);
        return std::make_unique<SlokedWin32File>(path);
    }

    void SlokedWin32File::ListFiles(FileVisitor visitor) const {
        WIN32_FIND_DATA ffd;
		HANDLE hFind;
        for (hFind = FindFirstFile(this->path.c_str(), &ffd); hFind != INVALID_HANDLE_VALUE && FindNextFile(hFind, &ffd);) {
            std::string str(ffd.cFileName);
            if (str != ".." && str != ".") {
                visitor(str);
            }
        }
		FindClose(hFind);
    }

    static void TraverseDir(const std::string &dir,
                            const SlokedFile::FileVisitor &visitor,
                            bool includes_dirs) {
        WIN32_FIND_DATA ffd;
		HANDLE hFind;
        for (hFind = FindFirstFile(dir.c_str(), &ffd); hFind != INVALID_HANDLE_VALUE && FindNextFile(hFind, &ffd);) {
            std::string path(ffd.cFileName);
            if (path != ".." && path != ".") {
                auto fullPath = dir + "/" + path;
                bool is_directory = ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
                if (!is_directory || includes_dirs) {
                    visitor(fullPath);
                }
                if (is_directory) {
                    TraverseDir(fullPath, visitor, includes_dirs);
                }
            }
        }
		FindClose(hFind);
    }

    void SlokedWin32File::Traverse(FileVisitor visitor,
                                   bool include_dirs) const {
        if (this->IsFile()) {
            visitor(this->path);
        } else {
            TraverseDir(this->path, visitor, include_dirs);
        }
    }
}  // namespace sloked