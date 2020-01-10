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

#include "sloked/facade/Crypto.h"

namespace sloked {


    SlokedCryptoFacade::SlokedCryptoFacade(SlokedCrypto &crypto)
        : crypto(crypto), credentials{std::unique_ptr<SlokedCredentialMaster>(nullptr)}, authenticator{} {}

    SlokedCrypto &SlokedCryptoFacade::GetEngine() const {
        return this->crypto;
    }

    bool SlokedCryptoFacade::HasCredentialMaster() const {
        return this->credentials.index() == 0 && std::get<0>(this->credentials) != nullptr;
    }

    SlokedCredentialMaster &SlokedCryptoFacade::GetCredentialMaster() const {
        if (this->HasCredentialMaster()) {
            return *std::get<0>(this->credentials);
        } else {
            throw SlokedError("CryptoFacade: Credential master not defined");
        }
    }

    bool SlokedCryptoFacade::HasCredentialSlave() const {
        return this->credentials.index() == 1 && std::get<1>(this->credentials) != nullptr;
    }

    SlokedCredentialSlave &SlokedCryptoFacade::GetCredentialSlave() const {
        if (this->HasCredentialSlave()) {
            return *std::get<1>(this->credentials);
        } else {
            throw SlokedError("CryptoFacade: Credential slave not defined");
        }
    }

    bool SlokedCryptoFacade::HasAuthenticator() const {
        return this->authenticator != nullptr;
    }

    SlokedAuthenticatorFactory &SlokedCryptoFacade::GetAuthenticator() const {
        if (this->HasAuthenticator()) {
            return *this->authenticator;
        } else {
            throw SlokedError("CryptoFacade: Authenticator not defined");
        }
    }

    SlokedCredentialMaster &SlokedCryptoFacade::SetupCredentialMaster(SlokedCrypto::Key &key) {
        auto master = std::make_unique<SlokedCredentialMaster>(this->crypto, key);
        auto &masterRef = *master;
        this->authenticator = nullptr;
        this->credentials = std::move(master);
        return masterRef;
    }

    SlokedCredentialSlave &SlokedCryptoFacade::SetupCredentialSlave() {
        auto slave = std::make_unique<SlokedCredentialSlave>(this->crypto);
        auto &slaveRef = *slave;
        this->authenticator = nullptr;
        this->credentials = std::move(slave);
        return slaveRef;
    }

    SlokedAuthenticatorFactory &SlokedCryptoFacade::SetupAuthenticator(const std::string &salt) {
        if (this->HasCredentialMaster()) {
            this->authenticator = std::make_unique<SlokedAuthenticatorFactory>(this->crypto, this->GetCredentialMaster(), salt);
        } else if (this->HasCredentialSlave()) {
            this->authenticator = std::make_unique<SlokedAuthenticatorFactory>(this->crypto, this->GetCredentialSlave(), salt);
        } else {
            throw SlokedError("CryptoFacade: Credentials not defined");
        }
        return *this->authenticator;
    }
}