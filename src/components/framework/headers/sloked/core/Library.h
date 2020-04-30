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

#ifndef SLOKED_CORE_LIBRARY_H_
#define SLOKED_CORE_LIBRARY_H_

#include <memory>
#include <string>

#include "sloked/Base.h"

namespace sloked {

    class SlokedDynamicLibrary {
     public:
        enum class Binding { Lazy, Now };

        enum class Scope { Global, Local };

        SlokedDynamicLibrary(const SlokedDynamicLibrary &) = delete;

        virtual ~SlokedDynamicLibrary() = default;
        SlokedDynamicLibrary &operator=(const SlokedDynamicLibrary &) = delete;

        virtual bool IsLoaded() const = 0;
        virtual const std::string GetPath() const = 0;
        virtual Binding GetBinding() const = 0;
        virtual Scope GetScope() const = 0;
        virtual void *Resolve(const std::string &) const;
        virtual void *Resolve(const char *) const = 0;
        virtual void Close() = 0;

     protected:
        SlokedDynamicLibrary() = default;
    };

    class SlokedDynamicLibraryLoader {
     public:
        virtual ~SlokedDynamicLibraryLoader() = default;
        virtual std::unique_ptr<SlokedDynamicLibrary> Load(
            const std::string &, SlokedDynamicLibrary::Binding,
            SlokedDynamicLibrary::Scope) const;
        virtual std::unique_ptr<SlokedDynamicLibrary> Load(
            const char *, SlokedDynamicLibrary::Binding,
            SlokedDynamicLibrary::Scope) const = 0;
    };
}  // namespace sloked

#endif