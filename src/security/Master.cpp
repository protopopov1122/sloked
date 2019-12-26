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

    const std::string &SlokedAuthenticationMaster::AccountToken::GetName() const {
        return this->name;
    }

    SlokedAuthenticationMaster::AccountToken &SlokedAuthenticationMaster::AccountToken::operator=(const AccountToken &token) {
        this->name = token.name;
        this->nonce = token.nonce;
        this->credentials = token.credentials;
        return *this;
    }

    SlokedAuthenticationMaster::AccountToken &SlokedAuthenticationMaster::AccountToken::operator=(AccountToken &&token) {
        this->name = std::move(token.name);
        this->nonce = token.nonce;
        token.nonce = {};
        this->credentials = std::move(token.credentials);
        return *this;
    }

    bool SlokedAuthenticationMaster::AccountToken::operator==(const AccountToken &token) const {
        return this->name == token.name &&
            this->nonce == token.nonce;
    }

    SlokedAuthenticationMaster::AccountToken::AccountToken(const SlokedAuthenticationMaster &auth, const std::string &name)
        : name(name), nonce(auth.NextNonce()), credentials{} {
        this->GenerateCredentials(auth);
    }

    SlokedAuthenticationMaster::AccountToken::AccountToken(const SlokedAuthenticationMaster &auth, std::string name, NonceType nonce)
        : name(std::move(name)), nonce(std::move(nonce)), credentials{} {
        this->GenerateCredentials(auth);
    }

    void SlokedAuthenticationMaster::AccountToken::GenerateCredentials(const SlokedAuthenticationMaster &auth) {
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

    SlokedAuthenticationMaster::Account::Account(SlokedAuthenticationMaster &auth, const std::string &name)
        : auth(auth), token(auth, name), nextWatcherId{0} {}

    SlokedAuthenticationMaster::Account::~Account() {
        std::unique_lock lock(this->mtx);
        this->token = AccountToken{this->auth, this->token.GetName()};
        for (auto it = this->watchers.begin(); it != this->watchers.end();) {
            auto current = it++;
            lock.unlock();
            current->second();
            lock.lock();
        }
    }

    const std::string &SlokedAuthenticationMaster::Account::GetName() const {
        std::unique_lock lock(this->mtx);
        return this->token.GetName();
    }

    void SlokedAuthenticationMaster::Account::RevokeCredentials() {
        std::unique_lock lock(this->mtx);
        this->token = AccountToken{this->auth, this->token.GetName()};
        for (auto it = this->watchers.begin(); it != this->watchers.end();) {
            auto current = it++;
            lock.unlock();
            current->second();
            lock.lock();
        }
    }

    const std::string &SlokedAuthenticationMaster::Account::GetCredentials() const {
        std::unique_lock lock(this->mtx);
        return this->token.credentials;
    }

    std::unique_ptr<SlokedCrypto::Key> SlokedAuthenticationMaster::Account::DeriveKey(const std::string &salt) const {
        std::unique_lock lock(this->mtx);
        return this->auth.crypto.DeriveKey(this->token.credentials, salt);
    }

    SlokedAuthenticationMaster::Account::Callback SlokedAuthenticationMaster::Account::Watch(Callback watcher) {
        std::unique_lock lock(this->mtx);
        int64_t watcherId = this->nextWatcherId++;
        this->watchers.emplace(watcherId, std::move(watcher));
        return [this, watcherId] {
            if (this->watchers.count(watcherId) != 0) {
                this->watchers.erase(watcherId);
            }
        };
    }

    bool SlokedAuthenticationMaster::Account::VerifyToken(const AccountToken &token) const {
        std::unique_lock lock(this->mtx);
        return token == this->token;
    }

    SlokedAuthenticationMaster::AccountToken SlokedAuthenticationMaster::Account::ParseCredentials(const SlokedAuthenticationMaster &auth, const std::string &credentials) {
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
        return SlokedAuthenticationMaster::AccountToken{auth, name, nonce};
    }

    SlokedAuthenticationMaster::SlokedAuthenticationMaster(SlokedCrypto &crypto, SlokedCrypto::Key &key)
        : crypto(crypto), cipher(crypto.NewCipher(key)), random(crypto.NewRandom()) {}

    SlokedAuthenticationMaster::Account &SlokedAuthenticationMaster::New(const std::string &name) {
        std::unique_lock lock(this->mtx);
        if (this->accounts.count(name) == 0) {
            auto account = std::make_unique<Account>(*this, name);
            this->accounts.emplace(name, std::move(account));
            return *this->accounts.at(name);
        } else {
            throw SlokedError("Auth: Account \'" + name + "\' already exists");
        }
    }

    bool SlokedAuthenticationMaster::Has(const std::string &name) const {
        std::unique_lock lock(this->mtx);
        return this->accounts.count(name) != 0;
    }

    SlokedAuthenticationMaster::Account &SlokedAuthenticationMaster::GetByName(const std::string &name) const {
        std::unique_lock lock(this->mtx);
        if (this->accounts.count(name) != 0) {
            return *this->accounts.at(name);
        } else {
            throw SlokedError("Auth: Account \'" + name + "\' doesn't exist");
        }
    }

    SlokedAuthenticationMaster::Account &SlokedAuthenticationMaster::GetByCredential(const std::string &credentials) const {
        std::unique_lock lock(this->mtx);
        auto token = Account::ParseCredentials(*this, credentials);
        auto &account = this->GetByName(token.GetName());
        if (account.VerifyToken(token)) {
            return account;
        } else {
            throw SlokedError("Auth: Invalid account \'" + account.GetName() + "\' credentials");
        }
    }

    SlokedAuthenticationMaster::AccountToken::NonceType SlokedAuthenticationMaster::NextNonce() const {
        AccountToken::NonceType nonce;
        for (std::size_t i = 0; i < nonce.size(); i++) {
            nonce[i] = this->random->NextByte();
        }
        return nonce;
    }

    std::string SlokedAuthenticationMaster::Encode(std::string_view data) const {
        SlokedCrypto::Data bytes(data.data(), data.data() + data.size());
        if (bytes.size() % this->cipher->BlockSize() != 0) {
            std::size_t zeros = this->cipher->BlockSize() - (bytes.size() % this->cipher->BlockSize());
            bytes.insert(bytes.end(), zeros, '\0');
        }
        auto encrypted = this->cipher->Encrypt(std::move(bytes));
        return SlokedBase64::Encode(encrypted.data(), encrypted.data() + encrypted.size());
    }

    std::string SlokedAuthenticationMaster::Decode(std::string_view data) const {
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