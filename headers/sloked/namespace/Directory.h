/*
  SPDX-License-Identifier: LGPL-3.0-or-later

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_NAMESPACE_DIRECTORY_H_
#define SLOKED_NAMESPACE_DIRECTORY_H_

#include "sloked/namespace/Object.h"

namespace sloked {

    class SlokedNamespaceDefaultDirectory : public SlokedNamespaceDirectory {
     public:
        SlokedNamespaceDefaultDirectory(SlokedNamespace &, const SlokedPath &);

        Type GetType() const override;
        const SlokedPath &GetPath() const override;
        SlokedNamespaceDirectory *AsDirectory() override;

        std::unique_ptr<SlokedNamespaceObject> GetObject(const SlokedPath &) override;
        bool HasObject(const SlokedPath &) const override;
        void Iterate(const SlokedPath &, Visitor) const override;
        std::unique_ptr<SlokedNamespaceObjectHandle> GetHandle(const SlokedPath &) override;

     private:
        SlokedNamespace &ns;
        SlokedPath path;
    };
}

#endif