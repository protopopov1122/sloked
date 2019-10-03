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

#ifndef SLOKED_NAMESPACE_FILESYSTEM_H_
#define SLOKED_NAMESPACE_FILESYSTEM_H_

#include "sloked/namespace/Object.h"
#include "sloked/filesystem/File.h"

namespace sloked {

   class SlokedFilesystemAdapter {
   public:
      virtual ~SlokedFilesystemAdapter() = default;
      virtual const SlokedPath &GetRoot() const = 0;
      virtual std::unique_ptr<SlokedFile> NewFile(const SlokedPath &) const = 0;
      virtual SlokedPath ToPath(const std::string &) const = 0;
   };

   class SlokedFilesystemNamespace : public SlokedNamespace {
   public:
      SlokedFilesystemNamespace(std::unique_ptr<SlokedFilesystemAdapter>);
      std::unique_ptr<SlokedNamespaceObject> GetObject(const SlokedPath &) override;
      bool HasObject(const SlokedPath &) const override;
      void Iterate(const SlokedPath &, Visitor) const override;
      void Traverse(const SlokedPath &, Visitor, bool = false) const override;
      std::unique_ptr<SlokedNamespaceObjectHandle> GetHandle(const SlokedPath &) override;

   private:
      std::unique_ptr<SlokedFilesystemAdapter> filesystem;
   };
}

#endif