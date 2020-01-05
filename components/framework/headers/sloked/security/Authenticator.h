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

#ifndef SLOKED_SECUTIRY_AUTHENTICATOR_H_
#define SLOKED_SECUTIRY_AUTHENTICATOR_H_

#include "sloked/net/Socket.h"
#include "sloked/core/Crypto.h"
#include "sloked/security/Provider.h"

namespace sloked {

    class SlokedBaseAuthenticator {
     public:
        using Challenge = uint32_t;
        SlokedBaseAuthenticator(SlokedCrypto &, SlokedCredentialProvider &, std::string, SlokedSocketEncryption *);
        virtual ~SlokedBaseAuthenticator();
        bool IsLoggedIn() const;
        std::string GetAccount() const;

     protected:
        std::unique_ptr<SlokedCrypto::Cipher> DeriveCipher(const std::string &);
        std::string GenerateToken(SlokedCrypto::Cipher &, Challenge);
        void SetupEncryption();

        SlokedCrypto &crypto;
        SlokedCredentialProvider &provider;
        const std::string salt;
        SlokedSocketEncryption *encryption;
        std::optional<std::string> account;
        SlokedCredentialProvider::Account::Callback unwatchCredentials;
    };

    class SlokedMasterAuthenticator : public SlokedBaseAuthenticator {
     public:
        SlokedMasterAuthenticator(SlokedCrypto &, SlokedCredentialProvider &, std::string, SlokedSocketEncryption * = nullptr);
        ~SlokedMasterAuthenticator();
        Challenge InitiateLogin();
        bool ContinueLogin(const std::string &, const std::string &);
        void FinalizeLogin();

     private:
        std::unique_ptr<SlokedCrypto::Random> random;
        std::optional<Challenge> nonce;
    };

    class SlokedSlaveAuthenticator : public SlokedBaseAuthenticator {
     public:
        using Challenge = SlokedMasterAuthenticator::Challenge;
        SlokedSlaveAuthenticator(SlokedCrypto &, SlokedCredentialProvider &, std::string, SlokedSocketEncryption * = nullptr);
        ~SlokedSlaveAuthenticator();
        std::string InitiateLogin(const std::string, Challenge);
        void ContinueLogin(const std::string &);
        void FinalizeLogin();
    };

    class SlokedAuthenticatorFactory {
     public:
        SlokedAuthenticatorFactory(SlokedCrypto &, SlokedCredentialProvider &, std::string);
        std::unique_ptr<SlokedMasterAuthenticator> NewMaster(SlokedSocketEncryption * = nullptr);
        std::unique_ptr<SlokedSlaveAuthenticator> NewSlave(SlokedSocketEncryption * = nullptr);

     private:
        SlokedCrypto &crypto;
        SlokedCredentialProvider &provider;
        std::string salt;
    };
}

#endif