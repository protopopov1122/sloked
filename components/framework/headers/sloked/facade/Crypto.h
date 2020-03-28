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

#ifndef SLOKED_FACADE_CRYPTO_H_
#define SLOKED_FACADE_CRYPTO_H_

#include <variant>

#include "sloked/core/Crypto.h"
#include "sloked/security/Authenticator.h"
#include "sloked/security/Master.h"
#include "sloked/security/Slave.h"

namespace sloked {

    class SlokedCryptoFacade {
     public:
        SlokedCryptoFacade(SlokedCrypto &);
        ~SlokedCryptoFacade();
        SlokedCrypto &GetEngine() const;
        bool HasCredentialMaster() const;
        SlokedCredentialMaster &GetCredentialMaster() const;
        bool HasCredentialSlave() const;
        SlokedCredentialSlave &GetCredentialSlave() const;
        bool HasAuthenticator() const;
        SlokedAuthenticatorFactory &GetAuthenticator() const;

        SlokedCredentialMaster &SetupCredentialMaster(SlokedCrypto::Key &);
        SlokedCredentialSlave &SetupCredentialSlave();
        SlokedAuthenticatorFactory &SetupAuthenticator(const std::string &);

     private:
        SlokedCrypto &crypto;
        std::variant<std::unique_ptr<SlokedCredentialMaster>,
                     std::unique_ptr<SlokedCredentialSlave>>
            credentials;
        std::unique_ptr<SlokedAuthenticatorFactory> authenticator;
    };
}  // namespace sloked

#endif