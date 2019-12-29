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

#include "sloked/security/Master.h"
#include "sloked/core/Error.h"
#include "sloked/core/Base64.h"
#include <sstream>

namespace sloked {

    const std::string &SlokedCredentialMaster::AccountToken::GetName() const {
        return this->name;
    }

    SlokedCredentialMaster::AccountToken &SlokedCredentialMaster::AccountToken::operator=(const AccountToken &token) {
        this->name = token.name;
        this->nonce = token.nonce;
        this->credentials = token.credentials;
        return *this;
    }

    SlokedCredentialMaster::AccountToken &SlokedCredentialMaster::AccountToken::operator=(AccountToken &&token) {
        this->name = std::move(token.name);
        this->nonce = token.nonce;
        token.nonce = {};
        this->credentials = std::move(token.credentials);
        return *this;
    }

    bool SlokedCredentialMaster::AccountToken::operator==(const AccountToken &token) const {
        return this->name == token.name &&
            this->nonce == token.nonce;
    }

    SlokedCredentialMaster::AccountToken::AccountToken(const SlokedCredentialMaster &auth, const std::string &name)
        : name(name), nonce(auth.NextNonce()), credentials{} {
        this->GenerateCredentials(auth);
    }

    SlokedCredentialMaster::AccountToken::AccountToken(const SlokedCredentialMaster &auth, std::string name, NonceType nonce)
        : name(std::move(name)), nonce(std::move(nonce)), credentials{} {
        this->GenerateCredentials(auth);
    }

    void SlokedCredentialMaster::AccountToken::GenerateCredentials(const SlokedCredentialMaster &auth) {
        std::stringstream ss;
        for (std::size_t i = 0; i < this->nonce.size(); i++) {
            ss << std::to_string(this->nonce[i]);
            if (i + 1 < this->nonce.size()) {
                ss << ',';
            }
        }
        ss << ':' << this->name;
        this->credentials = auth.Encode(ss.str());
    }

    SlokedCredentialMaster::Account::Account(SlokedCredentialMaster &auth, const std::string &name)
        : auth(auth), token(auth, name), nextWatcherId{0},
          accessRestrictions(SlokedNamedBlacklist::Make({})), modificationRestrictions(SlokedNamedBlacklist::Make({})) {}

    SlokedCredentialMaster::Account::~Account() {
        std::unique_lock lock(this->mtx);
        this->TriggerWatchers(lock);
    }

    std::string SlokedCredentialMaster::Account::GetName() const {
        std::unique_lock lock(this->mtx);
        return this->token.GetName();
    }

    void SlokedCredentialMaster::Account::RevokeCredentials() {
        std::unique_lock lock(this->mtx);
        this->token = AccountToken{this->auth, this->token.GetName()};
        this->TriggerWatchers(lock);
    }

    std::string SlokedCredentialMaster::Account::GetCredentials() const {
        std::unique_lock lock(this->mtx);
        return this->token.credentials;
    }

    std::unique_ptr<SlokedCrypto::Key> SlokedCredentialMaster::Account::DeriveKey(const std::string &salt) const {
        std::unique_lock lock(this->mtx);
        return this->auth.crypto.DeriveKey(this->token.credentials, salt);
    }

    SlokedCredentialMaster::Account::Callback SlokedCredentialMaster::Account::Watch(Callback watcher) {
        std::unique_lock lock(this->mtx);
        int64_t watcherId = this->nextWatcherId++;
        this->watchers.emplace(watcherId, std::move(watcher));
        return [this, watcherId] {
            std::unique_lock lock(this->mtx);
            if (this->watchers.count(watcherId) != 0) {
                this->watchers.erase(watcherId);
            }
        };
    }

    std::shared_ptr<SlokedNamedRestrictions> SlokedCredentialMaster::Account::GetAccessRestrictions() const {
        std::unique_lock lock(this->mtx);
        return this->accessRestrictions;
    }

    std::shared_ptr<SlokedNamedRestrictions> SlokedCredentialMaster::Account::GetModificationRestrictiions() const {
        std::unique_lock lock(this->mtx);
        return this->modificationRestrictions;
    }
    
    void SlokedCredentialMaster::Account::SetAccessRestrictions(std::shared_ptr<SlokedNamedRestrictions> restrictions) {
        std::unique_lock lock(this->mtx);
        this->accessRestrictions = restrictions;
    }

    void SlokedCredentialMaster::Account::SetModificationRestrictions(std::shared_ptr<SlokedNamedRestrictions> restrictions) {
        std::unique_lock lock(this->mtx);
        this->modificationRestrictions = restrictions;
    }

    bool SlokedCredentialMaster::Account::VerifyToken(const AccountToken &token) const {
        std::unique_lock lock(this->mtx);
        return token == this->token;
    }

    SlokedCredentialMaster::AccountToken SlokedCredentialMaster::Account::ParseCredentials(const SlokedCredentialMaster &auth, const std::string &credentials) {
        auto data = auth.Decode(credentials);
        auto separator = data.find(':');
        if (separator == data.npos) {
            throw SlokedError("Auth: Invalid token");
        }
        std::string rawNonce = data.substr(0, separator);
        const std::string &name = data.substr(separator + 1);
        AccountToken::NonceType nonce;
        std::size_t lastSeparator = 0;
        for (std::size_t i = 0; i < nonce.size(); i++) {
            if (lastSeparator == rawNonce.npos) {
                throw SlokedError("Auth: Invalid token");
            }
            separator = rawNonce.find(',', lastSeparator);
            nonce[i] = std::stoul(rawNonce.substr(lastSeparator, separator));
            lastSeparator = separator + 1;
        }
        return SlokedCredentialMaster::AccountToken{auth, name, nonce};
    }

    void SlokedCredentialMaster::Account::TriggerWatchers(std::unique_lock<std::mutex> &lock) {
        for (auto it = this->watchers.begin(); it != this->watchers.end();) {
            auto current = it++;
            lock.unlock();
            current->second();
            lock.lock();
        }
    }

    SlokedCredentialMaster::SlokedCredentialMaster(SlokedCrypto &crypto, SlokedCrypto::Key &key)
        : crypto(crypto), cipher(crypto.NewCipher(key)), random(crypto.NewRandom()),
          defaultAccount(std::make_shared<Account>(*this, "")) {}

    std::weak_ptr<SlokedCredentialMaster::Account> SlokedCredentialMaster::New(const std::string &name) {
        if (name.empty()) {
            throw SlokedError("CredentialMaster: Can't create account with empty name");
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
    
    std::weak_ptr<SlokedCredentialMaster::Account> SlokedCredentialMaster::EnableDefaultAccount(bool en) {
        if (en) {
            this->defaultAccount = std::make_shared<Account>(*this, "");
        } else {
            this->defaultAccount = nullptr;
        }
        return this->defaultAccount;
    }

    std::weak_ptr<SlokedCredentialMaster::Account> SlokedCredentialMaster::GetDefaultAccount() {
        return this->defaultAccount;
    }

    bool SlokedCredentialMaster::Has(const std::string &name) const {
        std::unique_lock lock(this->mtx);
        return this->accounts.count(name) != 0;
    }

    std::weak_ptr<SlokedCredentialProvider::Account> SlokedCredentialMaster::GetByName(const std::string &name) const {
        std::unique_lock lock(this->mtx);
        if (this->accounts.count(name) != 0) {
            return this->accounts.at(name);
        } else {
            throw SlokedError("Auth: Account \'" + name + "\' doesn't exist");
        }
    }

    std::weak_ptr<SlokedCredentialMaster::Account> SlokedCredentialMaster::GetAccountByName(const std::string &name) const {
        std::unique_lock lock(this->mtx);
        if (this->accounts.count(name) != 0) {
            return this->accounts.at(name);
        } else {
            throw SlokedError("Auth: Account \'" + name + "\' doesn't exist");
        }
    }

    SlokedCredentialMaster::Account &SlokedCredentialMaster::GetAccountByCredential(const std::string &credentials) const {
        std::unique_lock lock(this->mtx);
        auto token = Account::ParseCredentials(*this, credentials);
        auto account = this->GetAccountByName(token.GetName()).lock();
        if (account && account->VerifyToken(token)) {
            return *account;
        } else {
            throw SlokedError("Auth: Invalid account \'" + account->GetName() + "\' credentials");
        }
    }

    std::weak_ptr<SlokedNamedRestrictionAuthority::Account> SlokedCredentialMaster::GetRestrictionsByName(const std::string &name) {
        std::unique_lock lock(this->mtx);
        if (this->accounts.count(name) != 0) {
            return this->accounts.at(name);
        } else {
            throw SlokedError("Auth: Account \'" + name + "\' doesn't exist");
        }
    }

    std::weak_ptr<SlokedNamedRestrictionAuthority::Account> SlokedCredentialMaster::GetDefaultRestrictions() {
        return this->defaultAccount;
    }

    SlokedCredentialMaster::AccountToken::NonceType SlokedCredentialMaster::NextNonce() const {
        AccountToken::NonceType nonce;
        for (std::size_t i = 0; i < nonce.size(); i++) {
            nonce[i] = this->random->NextByte();
        }
        return nonce;
    }

    std::string SlokedCredentialMaster::Encode(std::string_view data) const {
        SlokedCrypto::Data bytes(data.data(), data.data() + data.size());
        if (bytes.size() % this->cipher->BlockSize() != 0) {
            std::size_t zeros = this->cipher->BlockSize() - (bytes.size() % this->cipher->BlockSize());
            bytes.insert(bytes.end(), zeros, '\0');
        }
        auto encrypted = this->cipher->Encrypt(std::move(bytes));
        return SlokedBase64::Encode(encrypted.data(), encrypted.data() + encrypted.size());
    }

    std::string SlokedCredentialMaster::Decode(std::string_view data) const {
        SlokedCrypto::Data encrypted = SlokedBase64::Decode(data);
        auto bytes = this->cipher->Decrypt(encrypted);
        std::string result{bytes.begin(), bytes.end()};
        auto zero = result.find('\0');
        if (zero != result.npos) {
            result.erase(zero);
        }
        return result;
    }
}