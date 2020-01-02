/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/namespace/View.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedNamespaceView::SlokedNamespaceView(SlokedNamespace &base, const SlokedPath &path)
        : base(base), root(path) {
        if (!root.IsAbsolute()) {
            root = root.RelativeTo(root.Root());
        }
    }

    std::unique_ptr<SlokedNamespaceObject> SlokedNamespaceView::GetObject(const SlokedPath &path) {
        return this->base.GetObject(this->MakePath(path));
    }

    bool SlokedNamespaceView::HasObject(const SlokedPath &path) const {
        return this->base.HasObject(this->MakePath(path));
    }

    void SlokedNamespaceView::Iterate(const SlokedPath &path, Visitor visitor) const {
        this->base.Iterate(this->MakePath(path), std::move(visitor));
    }

    void SlokedNamespaceView::Traverse(const SlokedPath &path, Visitor visitor, bool b) const {
        this->base.Traverse(this->MakePath(path), std::move(visitor), b);
    }

    std::unique_ptr<SlokedNamespaceObjectHandle> SlokedNamespaceView::GetHandle(const SlokedPath &path) {
        return this->base.GetHandle(this->MakePath(path));
    }

    SlokedPath SlokedNamespaceView::MakePath(const SlokedPath &path) const {
        SlokedPath realPath = path.IsAbsolute() ? path.RelativeTo(path.Root()) : path;
        SlokedPath result = realPath.RelativeTo(this->root);
        if (this->root.IsParent(result)) {
            return result;
        } else {
            throw SlokedError(std::string{"Path out of namespace scope: "} + path.ToString());
        }
    }
}