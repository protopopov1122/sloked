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

#include "sloked/namespace/Empty.h"

namespace sloked {

    class SlokedEmptyNamespaceObjectHandle
        : public SlokedNamespaceObjectHandle {
     public:
        std::optional<std::string> ToURI() const final {
            return {};
        }

        bool Exists() const {
            return false;
        }

        void MakeDir() final {}
        void MakeFile() final {}
        void Delete() final {}
        void Rename(const SlokedPath &) final {}

        bool HasPermission(SlokedNamespacePermission) const final {
            return false;
        }
    };

    std::unique_ptr<SlokedNamespaceObject> SlokedEmptyNamespace::GetObject(
        const SlokedPath &) {
        return nullptr;
    }

    bool SlokedEmptyNamespace::HasObject(const SlokedPath &) const {
        return false;
    }

    void SlokedEmptyNamespace::Iterate(const SlokedPath &, Visitor) const {}

    void SlokedEmptyNamespace::Traverse(const SlokedPath &, Visitor,
                                        bool) const {}

    std::unique_ptr<SlokedNamespaceObjectHandle>
        SlokedEmptyNamespace::GetHandle(const SlokedPath &) {
        return std::make_unique<SlokedEmptyNamespaceObjectHandle>();
    }
}  // namespace sloked