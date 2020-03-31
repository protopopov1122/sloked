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

#include "sloked/core/Library.h"

namespace sloked {

    void *SlokedDynamicLibrary::Resolve(const std::string &symbol) const {
        return this->Resolve(symbol.c_str());
    }

    std::unique_ptr<SlokedDynamicLibrary> SlokedDynamicLibraryLoader::Load(
        const std::string &path, SlokedDynamicLibrary::Binding binding,
        SlokedDynamicLibrary::Scope scope) const {
        return this->Load(path.c_str(), binding, scope);
    }
}  // namespace sloked