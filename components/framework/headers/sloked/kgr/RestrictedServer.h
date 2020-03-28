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

#ifndef SLOKED_SECURITY_RESTRICTEDSERVER_H_
#define SLOKED_SECURITY_RESTRICTEDSERVER_H_

#include "sloked/kgr/NamedServer.h"
#include "sloked/security/Restriction.h"

namespace sloked {

    class KgrRestrictedNamedServer : public KgrNamedServer,
                                     public SlokedNamedRestrictionTarget {
     public:
        KgrRestrictedNamedServer(
            KgrNamedServer &,
            std::shared_ptr<SlokedNamedRestrictions> = nullptr,
            std::shared_ptr<SlokedNamedRestrictions> = nullptr);
        void SetAccessRestrictions(
            std::shared_ptr<SlokedNamedRestrictions>) final;
        void SetModificationRestrictions(
            std::shared_ptr<SlokedNamedRestrictions>) final;
        std::unique_ptr<KgrPipe> Connect(const SlokedPath &) final;
        Connector GetConnector(const SlokedPath &) final;

        void Register(const SlokedPath &, std::unique_ptr<KgrService>) final;
        bool Registered(const SlokedPath &) final;
        void Deregister(const SlokedPath &) final;

     private:
        KgrNamedServer &server;
        std::shared_ptr<SlokedNamedRestrictions> accessRestrictions;
        std::shared_ptr<SlokedNamedRestrictions> modificationRestrictions;
    };
}  // namespace sloked

#endif