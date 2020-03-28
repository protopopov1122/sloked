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

#ifndef SLOKED_NAMESPACE_VIEW_H_
#define SLOKED_NAMESPACE_VIEW_H_

#include "sloked/namespace/Object.h"

namespace sloked {

    class SlokedNamespaceView : public SlokedNamespace {
     public:
        SlokedNamespaceView(SlokedNamespace &, const SlokedPath &);

        std::unique_ptr<SlokedNamespaceObject> GetObject(
            const SlokedPath &) override;
        bool HasObject(const SlokedPath &) const override;
        void Iterate(const SlokedPath &, Visitor) const override;
        void Traverse(const SlokedPath &, Visitor, bool = false) const override;
        std::unique_ptr<SlokedNamespaceObjectHandle> GetHandle(
            const SlokedPath &) override;

     private:
        SlokedPath MakePath(const SlokedPath &) const;

        SlokedNamespace &base;
        SlokedPath root;
    };
}  // namespace sloked

#endif