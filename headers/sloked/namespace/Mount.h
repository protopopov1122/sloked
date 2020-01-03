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

#ifndef SLOKED_NAMESPACE_MOUNT_H_
#define SLOKED_NAMESPACE_MOUNT_H_

#include "sloked/namespace/Filesystem.h"
#include "sloked/namespace/Object.h"
#include "sloked/core/URI.h"

namespace sloked {

    class SlokedMountableNamespace : public SlokedNamespace {
     public:
        virtual void Mount(const SlokedPath &, std::unique_ptr<SlokedNamespace>) = 0;
        virtual std::vector<SlokedPath> Mounted() const = 0;
        virtual void Umount(const SlokedPath &) = 0;
    };

    class SlokedNamespaceMounter {
     public:
        virtual ~SlokedNamespaceMounter() = default;
        virtual std::unique_ptr<SlokedNamespace> Mount(const SlokedUri &) const = 0;
    };

    class SlokedDefaultNamespaceMounter : public SlokedNamespaceMounter {
     public:
        SlokedDefaultNamespaceMounter(std::unique_ptr<SlokedFilesystemAdapter>, SlokedNamespace &);
        std::unique_ptr<SlokedNamespace> Mount(const SlokedUri &) const final;

     private:
        std::unique_ptr<SlokedFilesystemAdapter> filesystem;
        SlokedNamespace &root;
    };
}

#endif