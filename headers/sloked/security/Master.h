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

#ifndef SLOKED_SECURITY_MASTER_H_
#define SLOKED_SECURITY_MASTER_H_

#include "sloked/security/Provider.h"
#include <memory>
#include <map>
#include <array>
#include <mutex>

namespace sloked {

    class SlokedCredentialMaster : public SlokedCredentialProvider {
     public: class Account;
     private: class AccountToken {
         static constexpr std::size_t NonceSize = 16;
      public:
         const std::string &GetName() const;
         AccountToken &operator=(const AccountToken &);
         AccountToken &operator=(AccountToken &&);
         bool operator==(const AccountToken &) const;
         friend class Account;

         using NonceType = std::array<uint8_t, NonceSize>;

      private:
         AccountToken(const SlokedCredentialMaster &, const std::string &);
         AccountToken(const SlokedCredentialMaster &, std::string, NonceType);
         void GenerateCredentials(const SlokedCredentialMaster &);

         std::string name;
         NonceType nonce;
         std::string credentials;
     };
     friend class AccountToken;

     public:
        class Account : public SlokedCredentialProvider::Account {
         public:
            Account(SlokedCredentialMaster &, const std::string &);
            ~Account();
            const std::string &GetName() const;
            void RevokeCredentials();
            const std::string &GetCredentials() const final;
            std::unique_ptr<SlokedCrypto::Key> DeriveKey(const std::string &) const final;
            Callback Watch(Callback) final;

            friend class SlokedCredentialMaster;
         private:
            bool VerifyToken(const AccountToken &) const;
            static AccountToken ParseCredentials(const SlokedCredentialMaster &, const std::string &);

            SlokedCredentialMaster &auth;
            mutable std::mutex mtx;
            AccountToken token;
            uint64_t nextWatcherId;
            std::map<uint64_t, Callback> watchers;
        };
        friend class Account;

        SlokedCredentialMaster(SlokedCrypto &, SlokedCrypto::Key &);
        Account &New(const std::string &);
        bool Has(const std::string &) const final;
        Account &GetByName(const std::string &) const final;
        Account &GetByCredential(const std::string &) const;

     private:
        AccountToken::NonceType NextNonce() const;
        std::string Encode(std::string_view) const;
        std::string Decode(std::string_view) const;

        SlokedCrypto &crypto;
        mutable std::mutex mtx;
        std::unique_ptr<SlokedCrypto::Cipher> cipher;
        std::unique_ptr<SlokedCrypto::Random> random;
        std::map<std::string, std::unique_ptr<Account>> accounts;
    };
}

#endif