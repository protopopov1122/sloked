/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#ifndef SLOKED_NAMESPACE_OBJECT_H_
#define SLOKED_NAMESPACE_OBJECT_H_

#include "sloked/Base.h"
#include "sloked/core/IO.h"
#include "sloked/core/Permission.h"
#include "sloked/namespace/Path.h"
#include <memory>
#include <string>
#include <functional>

namespace sloked {

    class SlokedNamespaceFile;
    class SlokedNamespaceDirectory;
    class SlokedNamespace;

    class SlokedNamespaceObject {
     public:
        enum class Type {
            File,
            Directory,
            None
        };

        virtual ~SlokedNamespaceObject() = default;
        virtual Type GetType() const = 0;
        virtual const SlokedPath &GetPath() const = 0;
        virtual SlokedNamespaceFile *AsFile();
        virtual SlokedNamespaceDirectory *AsDirectory();
    };

    class SlokedNamespaceFile : public SlokedNamespaceObject {
     public:
        virtual std::unique_ptr<SlokedIOReader> Reader() const = 0;
        virtual std::unique_ptr<SlokedIOWriter> Writer() = 0;
        virtual std::unique_ptr<SlokedIOView> View() const = 0;
    };

    enum class SlokedNamespacePermission {
        Read = 0,
        Write
    };

    class SlokedNamespaceObjectHandle : public SlokedPermissionAuthority<SlokedNamespacePermission> {
     public:
        virtual ~SlokedNamespaceObjectHandle() = default;
        virtual std::optional<std::string> ToURI() const = 0;
        virtual bool Exists() const = 0;
        virtual void MakeDir() = 0;
        virtual void MakeFile() = 0;
        virtual void Delete() = 0;
        virtual void Rename(const SlokedPath &) = 0;
    };

    class SlokedNamespace {
     public:
        using Visitor = std::function<void(const std::string &, SlokedNamespaceObject::Type)>;

        virtual ~SlokedNamespace() = default;
        virtual std::unique_ptr<SlokedNamespaceObject> GetObject(const SlokedPath &) = 0;
        virtual bool HasObject(const SlokedPath &) const = 0;
        virtual void Iterate(const SlokedPath &, Visitor) const = 0;
        virtual void Traverse(const SlokedPath &, Visitor, bool = false) const = 0;
        virtual std::unique_ptr<SlokedNamespaceObjectHandle> GetHandle(const SlokedPath &) = 0;
    };

    class SlokedNamespaceDirectory : public SlokedNamespace, public SlokedNamespaceObject {};
}

#endif