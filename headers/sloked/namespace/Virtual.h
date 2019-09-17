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

#ifndef SLOKED_NAMESPACE_VIRTUAL_H_
#define SLOKED_NAMESPACE_VIRTUAL_H_

#include "sloked/namespace/Object.h"
#include <map>

namespace sloked {

    class SlokedVirtualNamespace : public SlokedNamespace {
     public:
        SlokedVirtualNamespace(std::unique_ptr<SlokedNamespace>);

        void Mount(const SlokedPath &, std::unique_ptr<SlokedNamespace>);
        void Umount(const SlokedPath &);

        std::unique_ptr<SlokedNamespaceObject> GetObject(const SlokedPath &) override;
        bool HasObject(const SlokedPath &) const override;
        void Iterate(const SlokedPath &, Visitor) const override;
        std::unique_ptr<SlokedNamespaceObjectHandle> GetHandle(const SlokedPath &) override;

        friend class SlokedVirtualObjectHandle;

     private:
        struct Entry {
            std::unique_ptr<SlokedNamespace> ns;
            SlokedPath path {"/"};
            std::map<std::string, Entry> subentries;
        };

        const Entry &find(const SlokedPath &) const;
        void cleanup(Entry &);
        Entry root;
    };
}

#endif