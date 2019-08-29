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

#include "sloked/namespace/Directory.h"

namespace sloked {

    SlokedNamespaceDefaultDirectory::SlokedNamespaceDefaultDirectory(SlokedNamespace &ns, const SlokedPath &path)
        : ns(ns), path(path) {}

    SlokedNamespaceObject::Type SlokedNamespaceDefaultDirectory::GetType() const {
        return Type::Directory;
    }

    const SlokedPath &SlokedNamespaceDefaultDirectory::GetPath() const {
        return this->path;
    }

    SlokedNamespaceDirectory *SlokedNamespaceDefaultDirectory::AsDirectory() {
        return this;
    }

    std::unique_ptr<SlokedNamespaceObject> SlokedNamespaceDefaultDirectory::GetObject(const SlokedPath &path) {
        if (path.IsAbsolute()) {
            return this->ns.GetObject(path);
        } else {
            return this->ns.GetObject(path.RelativeTo(this->path));
        }
    }

    bool SlokedNamespaceDefaultDirectory::HasObject(const SlokedPath &path) const {
        if (path.IsAbsolute()) {
            return this->ns.HasObject(path);
        } else {
            return this->ns.HasObject(path.RelativeTo(this->path));
        }
    }
    
    void SlokedNamespaceDefaultDirectory::Iterate(const SlokedPath &path, Visitor visitor) const {
        if (path.IsAbsolute()) {
            this->ns.Iterate(path, std::move(visitor));
        } else {
            this->ns.Iterate(path.RelativeTo(this->path), std::move(visitor));
        }
    }
    
    std::unique_ptr<SlokedNamespaceObjectHandle> SlokedNamespaceDefaultDirectory::GetHandle(const SlokedPath &path) {
        if (path.IsAbsolute()) {
            return this->ns.GetHandle(path);
        } else {
            return this->ns.GetHandle(path.RelativeTo(this->path));
        }
    }
}