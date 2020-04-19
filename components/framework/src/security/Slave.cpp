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

#include "sloked/security/Slave.h"

namespace sloked {

    SlokedCredentialSlave::Account::Account(SlokedCrypto &crypto,
                                            const std::string &identifier,
                                            const std::string &password)
        : crypto(crypto), identifier(identifier), password(password) {}

    SlokedCredentialSlave::Account::~Account() {
        this->changeEmitter.Emit();
    }

    std::string SlokedCredentialSlave::Account::GetIdentifier() const {
        std::unique_lock lock(this->mtx);
        return this->identifier;
    }

    std::string SlokedCredentialSlave::Account::GetPassword() const {
        std::unique_lock lock(this->mtx);
        return this->password;
    }

    std::unique_ptr<SlokedCrypto::Key>
        SlokedCredentialSlave::Account::DeriveKey(
            const std::string &salt) const {
        std::unique_lock lock(this->mtx);
        return this->crypto.DeriveKey(this->password, salt);
    }

    SlokedCredentialStorage::Account::Callback
        SlokedCredentialSlave::Account::Watch(Callback watcher) {
        return this->changeEmitter.Listen(std::move(watcher));
    }

    void SlokedCredentialSlave::Account::ChangeCredentials(std::string cred) {
        std::unique_lock lock(this->mtx);
        this->password = std::move(cred);
        lock.unlock();
        this->changeEmitter.Emit();
    }

    SlokedCredentialSlave::SlokedCredentialSlave(SlokedCrypto &crypto)
        : crypto(crypto) {}

    std::shared_ptr<SlokedCredentialSlave::Account> SlokedCredentialSlave::New(
        const std::string &name, const std::string &credentials) {
        std::unique_lock lock(this->mtx);
        if (this->accounts.count(name) == 0) {
            auto account =
                std::make_shared<Account>(this->crypto, name, credentials);
            this->accounts.emplace(name, std::move(account));
            return this->accounts.at(name);
        } else {
            throw SlokedError("SlaveAuthenticator: Account \'" + name +
                              "\' already exists");
        }
    }

    bool SlokedCredentialSlave::Has(const std::string &name) const {
        std::unique_lock lock(this->mtx);
        return this->accounts.count(name) != 0;
    }

    std::shared_ptr<SlokedCredentialStorage::Account>
        SlokedCredentialSlave::GetByName(const std::string &name) const {
        std::unique_lock lock(this->mtx);
        if (this->accounts.count(name) != 0) {
            return this->accounts.at(name);
        } else {
            throw SlokedError("SlaveAuthenticator: Account \'" + name +
                              "\' doesn't exist");
        }
    }

    std::shared_ptr<SlokedCredentialSlave::Account>
        SlokedCredentialSlave::GetAccountByName(const std::string &name) const {
        std::unique_lock lock(this->mtx);
        if (this->accounts.count(name) != 0) {
            return this->accounts.at(name);
        } else {
            throw SlokedError("SlaveAuthenticator: Account \'" + name +
                              "\' doesn't exist");
        }
    }
}  // namespace sloked