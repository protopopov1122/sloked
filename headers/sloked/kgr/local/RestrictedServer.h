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

#ifndef SLOKED_KGR_LOCAL_RESTRICTEDSERVER_H_
#define SLOKED_KGR_LOCAL_RESTRICTEDSERVER_H_

#include "sloked/kgr/NamedServer.h"
#include <vector>

namespace sloked {

    class KgrNamedRestrictions {
     public:
        virtual ~KgrNamedRestrictions() = default;
        virtual bool IsAllowed(const std::string &) const = 0;
    };

    class KgrNamedWhitelist : public KgrNamedRestrictions {
     public:
        KgrNamedWhitelist(std::vector<std::string> = {});
        bool IsAllowed(const std::string &) const final;
        static std::unique_ptr<KgrNamedWhitelist> Make(std::vector<std::string>);

     private:
        std::vector<std::string> prefixes;
    };

    class KgrNamedBlacklist : public KgrNamedRestrictions {
     public:
        KgrNamedBlacklist(std::vector<std::string> = {});
        bool IsAllowed(const std::string &) const final;
        static std::unique_ptr<KgrNamedBlacklist> Make(std::vector<std::string>);

     private:
        std::vector<std::string> prefixes;
    };

    class KgrNamedRestrictionManager {
     public:
        virtual ~KgrNamedRestrictionManager() = default;
        virtual void SetAccessRestrictions(std::shared_ptr<KgrNamedRestrictions>) = 0;
        virtual void SetModificationRestrictions(std::shared_ptr<KgrNamedRestrictions>) = 0;
    };

    class KgrRestrictedNamedServer : public KgrNamedServer, public KgrNamedRestrictionManager {
     public:
        KgrRestrictedNamedServer(KgrNamedServer &, std::shared_ptr<KgrNamedRestrictions> = nullptr, std::shared_ptr<KgrNamedRestrictions> = nullptr);
        void SetAccessRestrictions(std::shared_ptr<KgrNamedRestrictions>) final;
        void SetModificationRestrictions(std::shared_ptr<KgrNamedRestrictions>) final;
        std::unique_ptr<KgrPipe> Connect(const std::string &) final;
        Connector GetConnector(const std::string &) final;

        void Register(const std::string &, std::unique_ptr<KgrService>) final;
        bool Registered(const std::string &) final;
        void Deregister(const std::string &) final;

     private:
        KgrNamedServer &server;
        std::shared_ptr<KgrNamedRestrictions> accessRestrictions;
        std::shared_ptr<KgrNamedRestrictions> modificationRestrictions;
    };
}

#endif