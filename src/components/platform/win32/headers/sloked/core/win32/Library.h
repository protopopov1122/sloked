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

#ifndef SLOKED_CORE_WIN32_LIBRARY_H_
#define SLOKED_CORE_WIN32_LIBRARY_H_

#include "sloked/core/Library.h"
#include <windows.h>

namespace sloked {

    class SlokedWin32DynamicLibrary : public SlokedDynamicLibrary {
     public:
        SlokedWin32DynamicLibrary(const char *);
        SlokedWin32DynamicLibrary(SlokedWin32DynamicLibrary &&);
        ~SlokedWin32DynamicLibrary();

        SlokedWin32DynamicLibrary &operator=(SlokedWin32DynamicLibrary &&);

        bool IsLoaded() const final;
        const std::string GetPath() const final;
        Binding GetBinding() const final;
        Scope GetScope() const final;
        void *Resolve(const char *) const final;
        void Close() final;

     private:
        std::string path;
        HINSTANCE library;
    };

    class SlokedWin32DynamicLibraryLoader : public SlokedDynamicLibraryLoader {
     public:
        std::unique_ptr<SlokedDynamicLibrary> Load(
            const char *, SlokedDynamicLibrary::Binding,
            SlokedDynamicLibrary::Scope) const final;
    };
}  // namespace sloked

#endif