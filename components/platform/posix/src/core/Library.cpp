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

#include "sloked/core/posix/Library.h"

#include <dlfcn.h>

#include "sloked/core/Error.h"

namespace sloked {

    const char *DlError() {
        const char *result = dlerror();
        return result ? result : "Unknown";
    }

    SlokedPosixDynamicLibrary::SlokedPosixDynamicLibrary(const char *path,
                                                         Binding binding,
                                                         Scope scope)
        : path{path}, binding{binding}, scope{scope}, library{nullptr} {
        int mode = binding == Binding::Lazy ? RTLD_LAZY : RTLD_NOW;
        mode |= scope == Scope::Global ? RTLD_GLOBAL : RTLD_LAZY;
        this->library = dlopen(path, mode);
        if (this->library == NULL) {
            throw SlokedError(
                std::string{"PosixDynamicLibrary: Loading error \'"} +
                DlError() + "\'");
        }
    }

    SlokedPosixDynamicLibrary::SlokedPosixDynamicLibrary(
        SlokedPosixDynamicLibrary &&library)
        : path{library.path}, binding{library.binding}, scope{library.scope},
          library{library.library} {
        library.library = nullptr;
        library.path.clear();
    }

    SlokedPosixDynamicLibrary::~SlokedPosixDynamicLibrary() {
        dlclose(this->library);
    }

    SlokedPosixDynamicLibrary &SlokedPosixDynamicLibrary::operator=(
        SlokedPosixDynamicLibrary &&library) {
        if (this->library && dlclose(this->library) != 0) {
            throw SlokedError(
                std::string{"PosixDynamicLibrary: Unloading error \'"} +
                DlError() + "\'");
        }
        this->path = library.path;
        this->binding = library.binding;
        this->scope = library.scope;
        this->library = library.library;
        library.library = nullptr;
        library.path.clear();
        return *this;
    }

    bool SlokedPosixDynamicLibrary::IsLoaded() const {
        return this->library != nullptr;
    }

    const std::string SlokedPosixDynamicLibrary::GetPath() const {
        return this->path;
    }

    SlokedDynamicLibrary::Binding SlokedPosixDynamicLibrary::GetBinding()
        const {
        return this->binding;
    }

    SlokedDynamicLibrary::Scope SlokedPosixDynamicLibrary::GetScope() const {
        return this->scope;
    }

    void *SlokedPosixDynamicLibrary::Resolve(const char *symbol) const {
        if (this->library == nullptr) {
            throw SlokedError("PosixDynamicLibrary: Not loaded");
        }
        dlerror();
        void *result = dlsym(this->library, symbol);
        auto error = dlerror();
        if (error) {
            throw SlokedError(
                std::string{"SlokedDynamicLibrary: Resolving error \'"} +
                error + "\'");
        } else {
            return result;
        }
    }

    void SlokedPosixDynamicLibrary::Close() {
        if (this->library && dlclose(this->library) != 0) {
            throw SlokedError(
                std::string{"PosixDynamicLibrary: Unloading error \'"} +
                DlError() + "\'");
        }
    }

    std::unique_ptr<SlokedDynamicLibrary> SlokedPosixDynamicLibraryLoader::Load(
        const char *path, SlokedDynamicLibrary::Binding binding,
        SlokedDynamicLibrary::Scope scope) const {
        return std::make_unique<SlokedPosixDynamicLibrary>(path, binding,
                                                           scope);
    }
}  // namespace sloked