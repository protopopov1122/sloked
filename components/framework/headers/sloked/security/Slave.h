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

#ifndef SLOKED_SECURITY_SLAVE_H_
#define SLOKED_SECURITY_SLAVE_H_

#include <map>
#include <mutex>

#include "sloked/core/Event.h"
#include "sloked/security/CredentialStorage.h"

namespace sloked {

    class SlokedCredentialSlave : public SlokedCredentialStorage {
     public:
        class Account : public SlokedCredentialStorage::Account {
         public:
            Account(SlokedCrypto &, const std::string &, const std::string &);
            ~Account();
            std::string GetIdentifier() const final;
            std::string GetPassword() const final;
            std::unique_ptr<SlokedCrypto::Key> DeriveKey(
                const std::string &) const final;
            Callback Watch(Callback) final;
            void ChangeCredentials(std::string);

         private:
            mutable std::mutex mtx;
            SlokedCrypto &crypto;
            const std::string identifier;
            std::string password;
            SlokedEventEmitter<void> changeEmitter;
        };

        SlokedCredentialSlave(SlokedCrypto &);
        std::shared_ptr<Account> New(const std::string &, const std::string &);
        bool Has(const std::string &) const final;
        std::shared_ptr<SlokedCredentialStorage::Account> GetByName(
            const std::string &) const final;
        std::shared_ptr<Account> GetAccountByName(const std::string &) const;

     private:
        SlokedCrypto &crypto;
        mutable std::mutex mtx;
        std::map<std::string, std::shared_ptr<Account>> accounts;
    };
}  // namespace sloked

#endif