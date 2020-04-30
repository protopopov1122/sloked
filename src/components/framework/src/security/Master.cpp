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

#include "sloked/security/Master.h"

#include <sstream>

#include "sloked/core/Base64.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedCredentialMaster::Account::Account(SlokedCredentialMaster &auth,
                                             const std::string &name)
        : auth(auth), identifier(name),
          accessRestrictions(SlokedNamedBlacklist::Make({})),
          modificationRestrictions(SlokedNamedBlacklist::Make({})) {
        this->RandomizePassword();
    }

    SlokedCredentialMaster::Account::Account(SlokedCredentialMaster &auth,
                                             const std::string &name,
                                             const std::string &password)
        : auth(auth), identifier(name), password(password),
          accessRestrictions(SlokedNamedBlacklist::Make({})),
          modificationRestrictions(SlokedNamedBlacklist::Make({})) {}

    SlokedCredentialMaster::Account::~Account() {
        std::unique_lock lock(this->mtx);
        this->changeEmitter.Emit();
    }

    std::string SlokedCredentialMaster::Account::GetIdentifier() const {
        std::unique_lock lock(this->mtx);
        return this->identifier;
    }

    void SlokedCredentialMaster::Account::SetPassword(std::string password) {
        std::unique_lock lock(this->mtx);
        this->password = std::move(password);
        lock.unlock();
        this->changeEmitter.Emit();
    }

    void SlokedCredentialMaster::Account::RandomizePassword() {
        std::unique_lock lock(this->mtx);
        auto seed = this->auth.NextSeed();
        std::stringstream ss;
        for (std::size_t i = 0; i < seed.size(); i++) {
            ss << std::to_string(seed[i]);
            if (i + 1 < seed.size()) {
                ss << ':';
            }
        }
        ss << '/' << this->identifier;
        this->password = auth.Encode(ss.str());
        lock.unlock();
        this->changeEmitter.Emit();
    }

    std::string SlokedCredentialMaster::Account::GetPassword() const {
        std::unique_lock lock(this->mtx);
        return this->password;
    }

    std::unique_ptr<SlokedCrypto::Key>
        SlokedCredentialMaster::Account::DeriveKey(
            std::size_t keyLength, const std::string &salt) const {
        std::unique_lock lock(this->mtx);
        return this->auth.crypto.DeriveKey(keyLength, this->password, salt);
    }

    SlokedCredentialMaster::Account::Callback
        SlokedCredentialMaster::Account::Watch(Callback watcher) {
        return this->changeEmitter.Listen(std::move(watcher));
    }

    std::shared_ptr<SlokedNamedRestrictions>
        SlokedCredentialMaster::Account::GetAccessRestrictions() const {
        std::unique_lock lock(this->mtx);
        return this->accessRestrictions;
    }

    std::shared_ptr<SlokedNamedRestrictions>
        SlokedCredentialMaster::Account::GetModificationRestrictiions() const {
        std::unique_lock lock(this->mtx);
        return this->modificationRestrictions;
    }

    void SlokedCredentialMaster::Account::SetAccessRestrictions(
        std::shared_ptr<SlokedNamedRestrictions> restrictions) {
        std::unique_lock lock(this->mtx);
        this->accessRestrictions = restrictions;
    }

    void SlokedCredentialMaster::Account::SetModificationRestrictions(
        std::shared_ptr<SlokedNamedRestrictions> restrictions) {
        std::unique_lock lock(this->mtx);
        this->modificationRestrictions = restrictions;
    }

    SlokedCredentialMaster::SlokedCredentialMaster(SlokedCrypto &crypto,
                                                   SlokedCrypto::Key &key)
        : crypto(crypto), cipher(crypto.NewCipher()), key(key.Clone()),
          random(crypto.NewRandom()),
          defaultAccount(std::make_shared<Account>(*this, "")) {}

    std::shared_ptr<SlokedCredentialMaster::Account>
        SlokedCredentialMaster::New(const std::string &name) {
        if (name.empty()) {
            throw SlokedError(
                "CredentialMaster: Can't create account with empty name");
        }
        std::unique_lock lock(this->mtx);
        if (this->accounts.count(name) == 0) {
            auto account = std::make_shared<Account>(*this, name);
            this->accounts.emplace(name, std::move(account));
            return this->accounts.at(name);
        } else {
            throw SlokedError("Auth: Account \'" + name + "\' already exists");
        }
    }

    std::shared_ptr<SlokedCredentialMaster::Account>
        SlokedCredentialMaster::New(const std::string &name,
                                    const std::string &password) {
        if (name.empty()) {
            throw SlokedError(
                "CredentialMaster: Can't create account with empty name");
        }
        std::unique_lock lock(this->mtx);
        if (this->accounts.count(name) == 0) {
            auto account = std::make_shared<Account>(*this, name, password);
            this->accounts.emplace(name, std::move(account));
            return this->accounts.at(name);
        } else {
            throw SlokedError("Auth: Account \'" + name + "\' already exists");
        }
    }

    std::shared_ptr<SlokedCredentialMaster::Account>
        SlokedCredentialMaster::EnableDefaultAccount(bool en) {
        if (en) {
            this->defaultAccount = std::make_shared<Account>(*this, "");
        } else {
            this->defaultAccount = nullptr;
        }
        return this->defaultAccount;
    }

    std::shared_ptr<SlokedCredentialMaster::Account>
        SlokedCredentialMaster::GetDefaultAccount() {
        return this->defaultAccount;
    }

    bool SlokedCredentialMaster::Has(const std::string &name) const {
        std::unique_lock lock(this->mtx);
        return this->accounts.count(name) != 0;
    }

    std::shared_ptr<SlokedCredentialStorage::Account>
        SlokedCredentialMaster::GetByName(const std::string &name) const {
        std::unique_lock lock(this->mtx);
        if (this->accounts.count(name) != 0) {
            return this->accounts.at(name);
        } else {
            throw SlokedError("Auth: Account \'" + name + "\' doesn't exist");
        }
    }

    std::shared_ptr<SlokedCredentialMaster::Account>
        SlokedCredentialMaster::GetAccountByName(
            const std::string &name) const {
        std::unique_lock lock(this->mtx);
        if (this->accounts.count(name) != 0) {
            return this->accounts.at(name);
        } else {
            throw SlokedError("Auth: Account \'" + name + "\' doesn't exist");
        }
    }

    std::shared_ptr<SlokedNamedRestrictionAuthority::Account>
        SlokedCredentialMaster::GetRestrictionsByName(const std::string &name) {
        std::unique_lock lock(this->mtx);
        if (this->accounts.count(name) != 0) {
            return this->accounts.at(name);
        } else {
            throw SlokedError("Auth: Account \'" + name + "\' doesn't exist");
        }
    }

    std::shared_ptr<SlokedNamedRestrictionAuthority::Account>
        SlokedCredentialMaster::GetDefaultRestrictions() {
        return this->defaultAccount;
    }

    SlokedCredentialMaster::RandomSeed SlokedCredentialMaster::NextSeed()
        const {
        RandomSeed seed;
        for (std::size_t i = 0; i < seed.size(); i++) {
            seed[i] = this->random->NextByte();
        }
        return seed;
    }

    std::string SlokedCredentialMaster::Encode(std::string_view data) const {
        SlokedCrypto::Data bytes(data.data(), data.data() + data.size());
        if (bytes.size() % this->cipher->Parameters().BlockSize != 0) {
            std::size_t zeros =
                this->cipher->Parameters().BlockSize -
                (bytes.size() % this->cipher->Parameters().BlockSize);
            bytes.insert(bytes.end(), zeros, '\0');
        }
        SlokedCrypto::Data iv(this->cipher->Parameters().IVSize, 0);
        auto encrypted =
            this->cipher->Encrypt(std::move(bytes), *this->key, iv);
        return SlokedBase64::Encode(encrypted.data(),
                                    encrypted.data() + encrypted.size());
    }

    std::string SlokedCredentialMaster::Decode(std::string_view data) const {
        SlokedCrypto::Data encrypted = SlokedBase64::Decode(data);
        SlokedCrypto::Data iv(this->cipher->Parameters().IVSize, 0);
        auto bytes = this->cipher->Decrypt(encrypted, *this->key, iv);
        std::string result{bytes.begin(), bytes.end()};
        auto zero = result.find('\0');
        if (zero != result.npos) {
            result.erase(zero);
        }
        return result;
    }
}  // namespace sloked