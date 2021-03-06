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

#ifndef SLOKED_NAMESPACE_EMPTY_H_
#define SLOKED_NAMESPACE_EMPTY_H_

#include "sloked/namespace/Object.h"

namespace sloked {

    class SlokedEmptyNamespace : public SlokedNamespace {
     public:
        std::unique_ptr<SlokedNamespaceObject> GetObject(
            const SlokedPath &) final;
        bool HasObject(const SlokedPath &) const final;
        void Iterate(const SlokedPath &, Visitor) const final;
        void Traverse(const SlokedPath &, Visitor, bool = false) const final;
        std::unique_ptr<SlokedNamespaceObjectHandle> GetHandle(
            const SlokedPath &) final;
    };
}  // namespace sloked

#endif