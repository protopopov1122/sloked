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

#ifndef SLOKED_SECURITY_AUTH_H_
#define SLOKED_SECURITY_AUTH_H_

#include "sloked/core/Crypto.h"
#include <memory>
#include <map>

namespace sloked {

    class SlokedAuth {
     public: class Account;
     private: class AccountToken {
      public:
         const std::string &GetName() const;
         AccountToken &operator=(const AccountToken &);
         AccountToken &operator=(AccountToken &&);
         bool operator==(const AccountToken &) const;
         friend class Account;

      private:
         AccountToken(const SlokedAuth &, const std::string &);
         AccountToken(const SlokedAuth &, const std::string &, uint64_t);

         std::string name;
         uint64_t nonce;
         std::string credentials;
     };
     friend class AccountToken;

     public:
        class Account {
         public:
            Account(SlokedAuth &, const std::string &);
            const std::string &GetName() const;
            void RevokeCredentials();
            const std::string &GetCredentials() const;
            friend class SlokedAuth;

         private:
            bool VerifyToken(const AccountToken &) const;
            static AccountToken ParseCredentials(const SlokedAuth &, const std::string &);

            SlokedAuth &auth;
            AccountToken token;
        };
        friend class Account;

        SlokedAuth(std::unique_ptr<SlokedCrypto::Cipher>, std::unique_ptr<SlokedCrypto::Random>);
        Account &New(const std::string &);
        bool Has(const std::string &) const;
        Account &GetByName(const std::string &) const;
        Account &GetByCredential(const std::string &) const;

     private:
        uint64_t NextNonce() const;
        std::string Encode(std::string_view) const;
        std::string Decode(std::string_view) const;

        std::unique_ptr<SlokedCrypto::Cipher> cipher;
        std::unique_ptr<SlokedCrypto::Random> random;
        std::map<std::string, std::unique_ptr<Account>> accounts;
    };
}

#endif