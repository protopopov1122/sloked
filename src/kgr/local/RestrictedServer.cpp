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

#include "sloked/kgr/local/RestrictedServer.h"
#include "sloked/core/Error.h"
#include "sloked/core/String.h"

namespace sloked {

    static bool ContainsPrefix(const std::string &name, const std::vector<std::string> &prefixes) {
        for (const auto &prefix : prefixes) {
            if (starts_with(name, prefix)) {
                return true;
            }
        }
        return false;
    }

    KgrNamedWhitelist::KgrNamedWhitelist(std::vector<std::string> prefixes)
        : prefixes(std::move(prefixes)) {}

    bool KgrNamedWhitelist::IsAllowed(const std::string &name) const {
        return ContainsPrefix(name, this->prefixes);
    }

    std::unique_ptr<KgrNamedWhitelist> KgrNamedWhitelist::Make(std::vector<std::string> prefixes) {
        return std::make_unique<KgrNamedWhitelist>(std::move(prefixes));
    }

    KgrNamedBlacklist::KgrNamedBlacklist(std::vector<std::string> prefixes)
        : prefixes(prefixes) {}

    bool KgrNamedBlacklist::IsAllowed(const std::string &name) const {
        return !ContainsPrefix(name, this->prefixes);
    }

    std::unique_ptr<KgrNamedBlacklist> KgrNamedBlacklist::Make(std::vector<std::string> prefixes) {
        return std::make_unique<KgrNamedBlacklist>(std::move(prefixes));
    }

    KgrRestrictedNamedServer::KgrRestrictedNamedServer(KgrNamedServer &server, std::shared_ptr<KgrNamedRestrictions> access, std::shared_ptr<KgrNamedRestrictions> modification)
        : server(server), accessRestrictions(std::move(access)), modificationRestrictions(std::move(modification)) {}

    void KgrRestrictedNamedServer::SetAccessRestrictions(std::shared_ptr<KgrNamedRestrictions> restrictions) {
        this->accessRestrictions = std::move(restrictions);
    }

    void KgrRestrictedNamedServer::SetModificationRestrictions(std::shared_ptr<KgrNamedRestrictions> restrictions) {
        this->modificationRestrictions = std::move(restrictions);
    }

    std::unique_ptr<KgrPipe> KgrRestrictedNamedServer::Connect(const std::string &name) {
        if (this->accessRestrictions != nullptr && this->accessRestrictions->IsAllowed(name)) {
            return this->server.Connect(name);
        } else {
            throw SlokedError("KgrNamedServer: Connection to \'" + name + "\' restricted");
        }
    }

    KgrNamedServer::Connector KgrRestrictedNamedServer::GetConnector(const std::string &name) {
        return [this, name] {
            return this->Connect(name);
        };
    }

    void KgrRestrictedNamedServer::Register(const std::string &name, std::unique_ptr<KgrService> service) {
        if (this->modificationRestrictions != nullptr && this->modificationRestrictions->IsAllowed(name)) {
            this->server.Register(name, std::move(service));
        } else {
            throw SlokedError("KgrNamedServer: Modification restricted \'" + name + "\'");
        }
    }
    
    bool KgrRestrictedNamedServer::Registered(const std::string &name) {
        return ((this->accessRestrictions != nullptr && this->accessRestrictions->IsAllowed(name)) ||
            (this->modificationRestrictions != nullptr && this->modificationRestrictions->IsAllowed(name))) &&
            this->server.Registered(name);
    }

    void KgrRestrictedNamedServer::Deregister(const std::string &name) {
        if (this->modificationRestrictions != nullptr && this->modificationRestrictions->IsAllowed(name)) {
            this->server.Deregister(name);
        } else {
            throw SlokedError("KgrNamedServer: Modification restricted \'" + name + "\'");
        }
    }
}