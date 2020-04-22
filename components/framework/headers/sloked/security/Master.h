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

#ifndef SLOKED_SECURITY_MASTER_H_
#define SLOKED_SECURITY_MASTER_H_

#include <array>
#include <map>
#include <memory>
#include <mutex>

#include "sloked/core/Event.h"
#include "sloked/security/CredentialStorage.h"
#include "sloked/security/Restriction.h"

namespace sloked {

    class SlokedCredentialMaster : public SlokedCredentialStorage,
                                   public SlokedNamedRestrictionAuthority {
     public:
        class Account : public SlokedCredentialStorage::Account,
                        public SlokedNamedRestrictionAuthority::Account,
                        public SlokedNamedRestrictionTarget {
         public:
            Account(SlokedCredentialMaster &, const std::string &);
            Account(SlokedCredentialMaster &, const std::string &,
                    const std::string &);
            ~Account();
            std::string GetIdentifier() const;
            void SetPassword(std::string);
            void RandomizePassword();
            std::string GetPassword() const final;
            std::unique_ptr<SlokedCrypto::Key> DeriveKey(
                std::size_t, const std::string &) const final;
            Callback Watch(Callback) final;

            std::shared_ptr<SlokedNamedRestrictions> GetAccessRestrictions()
                const final;
            std::shared_ptr<SlokedNamedRestrictions>
                GetModificationRestrictiions() const final;

            void SetAccessRestrictions(
                std::shared_ptr<SlokedNamedRestrictions>) final;
            void SetModificationRestrictions(
                std::shared_ptr<SlokedNamedRestrictions>) final;

            friend class SlokedCredentialMaster;

         private:
            SlokedCredentialMaster &auth;
            mutable std::mutex mtx;
            const std::string identifier;
            std::string password;
            SlokedEventEmitter<void> changeEmitter;
            std::shared_ptr<SlokedNamedRestrictions> accessRestrictions;
            std::shared_ptr<SlokedNamedRestrictions> modificationRestrictions;
        };
        friend class Account;

        SlokedCredentialMaster(SlokedCrypto &, SlokedCrypto::Key &);
        std::shared_ptr<Account> New(const std::string &);
        std::shared_ptr<Account> New(const std::string &, const std::string &);
        std::shared_ptr<Account> EnableDefaultAccount(bool);
        std::shared_ptr<Account> GetDefaultAccount();
        std::shared_ptr<Account> GetAccountByName(const std::string &) const;

        bool Has(const std::string &) const final;
        std::shared_ptr<SlokedCredentialStorage::Account> GetByName(
            const std::string &) const final;

        std::shared_ptr<SlokedNamedRestrictionAuthority::Account>
            GetRestrictionsByName(const std::string &) final;
        std::shared_ptr<SlokedNamedRestrictionAuthority::Account>
            GetDefaultRestrictions() final;

     private:
        using RandomSeed = std::array<uint8_t, 16>;
        RandomSeed NextSeed() const;
        std::string Encode(std::string_view) const;
        std::string Decode(std::string_view) const;

        SlokedCrypto &crypto;
        mutable std::mutex mtx;
        std::unique_ptr<SlokedCrypto::Cipher> cipher;
        std::unique_ptr<SlokedCrypto::Key> key;
        std::unique_ptr<SlokedCrypto::Random> random;
        std::shared_ptr<Account> defaultAccount;
        std::map<std::string, std::shared_ptr<Account>> accounts;
    };
}  // namespace sloked

#endif