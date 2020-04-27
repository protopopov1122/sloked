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

#ifndef SLOKED_SECUTIRY_AUTHENTICATOR_H_
#define SLOKED_SECUTIRY_AUTHENTICATOR_H_

#include "sloked/core/Crypto.h"
#include "sloked/net/Socket.h"
#include "sloked/security/CredentialStorage.h"

namespace sloked {

    class SlokedBaseAuthenticator {
     public:
        using Challenge = uint32_t;
        SlokedBaseAuthenticator(SlokedCrypto &, SlokedCredentialStorage &,
                                std::string, SlokedSocketEncryption *);
        virtual ~SlokedBaseAuthenticator();
        bool IsLoggedIn() const;
        std::string GetAccount() const;
        void Logout();

     protected:
        std::unique_ptr<SlokedCrypto::Key> DeriveKey(std::size_t,
                                                     const std::string &);
        std::string GenerateToken(SlokedCrypto::Cipher &, SlokedCrypto::Key &,
                                  Challenge);
        void SetupEncryption(bool);

        SlokedCrypto &crypto;
        SlokedCredentialStorage &provider;
        const std::string salt;
        SlokedSocketEncryption *encryption;
        std::optional<std::string> account;
        SlokedCredentialStorage::Account::Callback unwatchCredentials;
        std::unique_ptr<SlokedCrypto::Key> initialKey;
    };

    class SlokedMasterAuthenticator : public SlokedBaseAuthenticator {
     public:
        SlokedMasterAuthenticator(SlokedCrypto &, SlokedCredentialStorage &,
                                  std::string,
                                  SlokedSocketEncryption * = nullptr);
        ~SlokedMasterAuthenticator();
        Challenge InitiateLogin();
        bool ContinueLogin(const std::string &, const std::string &);
        void FinalizeLogin();

     private:
        std::unique_ptr<SlokedCrypto::Cipher> cipher;
        std::unique_ptr<SlokedCrypto::Random> random;
        std::optional<Challenge> nonce;
    };

    class SlokedSlaveAuthenticator : public SlokedBaseAuthenticator {
     public:
        using Challenge = SlokedMasterAuthenticator::Challenge;
        SlokedSlaveAuthenticator(SlokedCrypto &, SlokedCredentialStorage &,
                                 std::string,
                                 SlokedSocketEncryption * = nullptr);
        ~SlokedSlaveAuthenticator();

        std::string InitiateLogin(const std::string, Challenge);
        void Close();

     private:
        std::unique_ptr<SlokedCrypto::Cipher> cipher;
        std::function<void()> unbindEncryptionListener;
        std::optional<std::string> pending;
    };

    class SlokedAuthenticatorFactory {
     public:
        SlokedAuthenticatorFactory(SlokedCrypto &, SlokedCredentialStorage &,
                                   std::string);
        std::unique_ptr<SlokedMasterAuthenticator> NewMaster(
            SlokedSocketEncryption * = nullptr);
        std::unique_ptr<SlokedSlaveAuthenticator> NewSlave(
            SlokedSocketEncryption * = nullptr);

     private:
        SlokedCrypto &crypto;
        SlokedCredentialStorage &provider;
        std::string salt;
    };
}  // namespace sloked

#endif