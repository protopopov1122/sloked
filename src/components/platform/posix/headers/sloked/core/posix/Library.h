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

#ifndef SLOKED_CORE_POSIX_LIBRARY_H_
#define SLOKED_CORE_POSIX_LIBRARY_H_

#include "sloked/core/Library.h"

namespace sloked {

    class SlokedPosixDynamicLibrary : public SlokedDynamicLibrary {
     public:
        SlokedPosixDynamicLibrary(const char *, Binding, Scope);
        SlokedPosixDynamicLibrary(SlokedPosixDynamicLibrary &&);
        ~SlokedPosixDynamicLibrary();

        SlokedPosixDynamicLibrary &operator=(SlokedPosixDynamicLibrary &&);

        bool IsLoaded() const final;
        const std::string GetPath() const final;
        Binding GetBinding() const final;
        Scope GetScope() const final;
        void *Resolve(const char *) const final;
        void Close() final;

     private:
        std::string path;
        Binding binding;
        Scope scope;
        void *library;
    };

    class SlokedPosixDynamicLibraryLoader : public SlokedDynamicLibraryLoader {
     public:
        std::unique_ptr<SlokedDynamicLibrary> Load(
            const char *, SlokedDynamicLibrary::Binding,
            SlokedDynamicLibrary::Scope) const final;
    };
}  // namespace sloked

#endif