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

#include "sloked/security/Auth.h"
#include "sloked/core/Error.h"
#include "sloked/core/Base64.h"
#include <sstream>

namespace sloked {

    const std::string &SlokedAuth::AccountToken::GetName() const {
        return this->name;
    }

    SlokedAuth::AccountToken &SlokedAuth::AccountToken::operator=(const AccountToken &token) {
        this->name = token.name;
        this->nonce = token.nonce;
        this->credentials = token.credentials;
        return *this;
    }

    SlokedAuth::AccountToken &SlokedAuth::AccountToken::operator=(AccountToken &&token) {
        this->name = std::move(token.name);
        this->nonce = token.nonce;
        token.nonce = {};
        this->credentials = std::move(token.credentials);
        return *this;
    }

    bool SlokedAuth::AccountToken::operator==(const AccountToken &token) const {
        return this->name == token.name &&
            this->nonce == token.nonce;
    }

    SlokedAuth::AccountToken::AccountToken(const SlokedAuth &auth, const std::string &name)
        : name(name), nonce(auth.NextNonce()), credentials{} {
        this->GenerateCredentials(auth);
    }

    SlokedAuth::AccountToken::AccountToken(const SlokedAuth &auth, std::string name, NonceType nonce)
        : name(std::move(name)), nonce(std::move(nonce)), credentials{} {
        this->GenerateCredentials(auth);
    }

    void SlokedAuth::AccountToken::GenerateCredentials(const SlokedAuth &auth) {
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

    SlokedAuth::Account::Account(SlokedAuth &auth, const std::string &name)
        : auth(auth), token(auth, name) {}

    const std::string &SlokedAuth::Account::GetName() const {
        return this->token.GetName();
    }

    void SlokedAuth::Account::RevokeCredentials() {
        this->token = AccountToken{this->auth, this->token.GetName()};
    }

    const std::string &SlokedAuth::Account::GetCredentials() const {
        return this->token.credentials;
    }

    bool SlokedAuth::Account::VerifyToken(const AccountToken &token) const {
        return token == this->token;
    }

    SlokedAuth::AccountToken SlokedAuth::Account::ParseCredentials(const SlokedAuth &auth, const std::string &credentials) {
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
        return SlokedAuth::AccountToken{auth, name, nonce};
    }

    SlokedAuth::SlokedAuth(std::unique_ptr<SlokedCrypto::Cipher> cipher, std::unique_ptr<SlokedCrypto::Random> random)
        : cipher(std::move(cipher)), random(std::move(random)) {}

    SlokedAuth::Account &SlokedAuth::New(const std::string &name) {
        if (this->accounts.count(name) == 0) {
            auto account = std::make_unique<Account>(*this, name);
            this->accounts.emplace(name, std::move(account));
            return *this->accounts.at(name);
        } else {
            throw SlokedError("Auth: Account \'" + name + "\' already exists");
        }
    }

    bool SlokedAuth::Has(const std::string &name) const {
        return this->accounts.count(name) != 0;
    }

    SlokedAuth::Account &SlokedAuth::GetByName(const std::string &name) const {
        if (this->accounts.count(name) != 0) {
            return *this->accounts.at(name);
        } else {
            throw SlokedError("Auth: Account \'" + name + "\' doesn't exist");
        }
    }

    SlokedAuth::Account &SlokedAuth::GetByCredential(const std::string &credentials) const {
        auto token = Account::ParseCredentials(*this, credentials);
        auto &account = this->GetByName(token.GetName());
        if (account.VerifyToken(token)) {
            return account;
        } else {
            throw SlokedError("Auth: Invalid account \'" + account.GetName() + "\' credentials");
        }
    }

    SlokedAuth::AccountToken::NonceType SlokedAuth::NextNonce() const {
        AccountToken::NonceType nonce;
        for (std::size_t i = 0; i < nonce.size(); i++) {
            nonce[i] = this->random->NextByte();
        }
        return nonce;
    }

    std::string SlokedAuth::Encode(std::string_view data) const {
        SlokedCrypto::Data bytes(data.data(), data.data() + data.size());
        if (bytes.size() % this->cipher->BlockSize() != 0) {
            std::size_t zeros = this->cipher->BlockSize() - (bytes.size() % this->cipher->BlockSize());
            bytes.insert(bytes.end(), zeros, '\0');
        }
        auto encrypted = this->cipher->Encrypt(std::move(bytes));
        return SlokedBase64::Encode(encrypted.data(), encrypted.data() + encrypted.size());
    }

    std::string SlokedAuth::Decode(std::string_view data) const {
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