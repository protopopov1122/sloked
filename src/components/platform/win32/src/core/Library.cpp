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

#include "sloked/core/win32/Library.h"

#include "sloked/core/Error.h"

namespace sloked {

    SlokedWin32DynamicLibrary::SlokedWin32DynamicLibrary(const char *path)
        : path{path}, library{nullptr} {
        this->library = LoadLibrary(TEXT(path));
        if (this->library == NULL) {
            throw SlokedError(
                "Win32DynamicLibrary: Loading error");
        }
    }

    SlokedWin32DynamicLibrary::SlokedWin32DynamicLibrary(
        SlokedWin32DynamicLibrary &&library)
        : path{library.path}, library{library.library} {
        library.library = nullptr;
        library.path.clear();
    }

    SlokedWin32DynamicLibrary::~SlokedWin32DynamicLibrary() {
        FreeLibrary(this->library);
    }

    SlokedWin32DynamicLibrary &SlokedWin32DynamicLibrary::operator=(
        SlokedWin32DynamicLibrary &&library) {
        if (this->library && FreeLibrary(this->library) != 0) {
            throw SlokedError(
                "Win32DynamicLibrary: Unloading error");
        }
        this->path = library.path;
        this->library = library.library;
        library.library = nullptr;
        library.path.clear();
        return *this;
    }

    bool SlokedWin32DynamicLibrary::IsLoaded() const {
        return this->library != nullptr;
    }

    const std::string SlokedWin32DynamicLibrary::GetPath() const {
        return this->path;
    }

    SlokedDynamicLibrary::Binding SlokedWin32DynamicLibrary::GetBinding()
        const {
        return Binding::Now;
    }

    SlokedDynamicLibrary::Scope SlokedWin32DynamicLibrary::GetScope() const {
        return Scope::Global;
    }

    void *SlokedWin32DynamicLibrary::Resolve(const char *symbol) const {
        if (this->library == nullptr) {
            throw SlokedError("Win32DynamicLibrary: Not loaded");
        }
        void *result = reinterpret_cast<void *>(GetProcAddress(this->library, symbol));
        if (result == nullptr) {
            throw SlokedError(
                "Win32DynamicLibrary: Resolving error");
        } else {
            return result;
        }
    }

    void SlokedWin32DynamicLibrary::Close() {
        if (this->library && FreeLibrary(this->library) != 0) {
            throw SlokedError(
                "Win32DynamicLibrary: Unloading error");
        }
    }

    std::unique_ptr<SlokedDynamicLibrary> SlokedWin32DynamicLibraryLoader::Load(
        const char *path, SlokedDynamicLibrary::Binding binding,
        SlokedDynamicLibrary::Scope scope) const {
        return std::make_unique<SlokedWin32DynamicLibrary>(path);
    }
}  // namespace slokeds