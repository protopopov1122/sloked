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

#include "sloked/security/Authenticator.h"
#include "sloked/core/Base64.h"

namespace sloked {

    SlokedMasterAuthenticator::SlokedMasterAuthenticator(SlokedCrypto &crypto, SlokedCredentialProvider &provider, std::string salt, SlokedSocketEncryption *encryption)
        : crypto(crypto), random(crypto.NewRandom()), provider(provider), salt(std::move(salt)), encryption(encryption) {}
    
    SlokedMasterAuthenticator::~SlokedMasterAuthenticator() {
        if (this->unwatchCredentials) {
            this->unwatchCredentials();
        }
    }

    bool SlokedMasterAuthenticator::IsLoggedIn() const {
        return this->account.has_value();
    }

    std::optional<std::string> SlokedMasterAuthenticator::GetAccount() const {
        return this->account;
    }

    SlokedMasterAuthenticator::Challenge SlokedMasterAuthenticator::InitiateLogin() {
        this->nonce = this->random->NextInt<Challenge>();
        this->account.reset();
        return this->nonce.value();
    }

    bool SlokedMasterAuthenticator::ContinueLogin(const std::string &account, const std::string &token) {
        if (!this->nonce.has_value()) {
            throw SlokedError("AuthenticationMaster: Initiate login first");
        }
        auto key = this->provider.GetByName(account).DeriveKey(this->salt);
        auto cipher = this->crypto.NewCipher(std::move(key));
        if (cipher->BlockSize() < sizeof(Challenge)) {
            throw SlokedError("AuthenticationMaster: Authentication not supported for current cipher");
        }
        constexpr std::size_t NonceSize = sizeof(Challenge) / sizeof(uint8_t);
        union {
            uint8_t bytes[NonceSize];
            uint32_t value;
        } nonce;
        nonce.value = this->nonce.value();
        this->nonce.reset();
        std::vector<uint8_t> raw;
        raw.insert(raw.end(), cipher->BlockSize(), 0);
        for (std::size_t i = 0; i < NonceSize; i++) {
            raw[i] = nonce.bytes[i];
        }
        auto encrypted = cipher->Encrypt(raw);
        auto expectedToken = SlokedBase64::Encode(encrypted.data(), encrypted.data() + encrypted.size());
        if (expectedToken == token) {
            this->account = account;
            return true;
        } else {
            return false;
        }
    }

    void SlokedMasterAuthenticator::SetupEncryption() {
        if (!this->account.has_value()) {
            throw SlokedError("AuthenticationMaster: Log in required");
        }
        if (this->unwatchCredentials) {
            this->unwatchCredentials();
            this->unwatchCredentials = nullptr;
        }
        if (this->encryption) {
            auto key = this->provider.GetByName(this->account.value()).DeriveKey(this->salt);
            auto cipher = this->crypto.NewCipher(std::move(key));
            this->encryption->SetEncryption(std::move(cipher));
            this->unwatchCredentials = this->provider.GetByName(this->account.value()).Watch([this] {
                if (this->account.has_value()) {
                    auto key = this->provider.GetByName(this->account.value()).DeriveKey(this->salt);
                    auto cipher = this->crypto.NewCipher(std::move(key));
                    this->encryption->SetEncryption(std::move(cipher));
                }
            });
        }
    }

    SlokedSlaveAuthenticator::SlokedSlaveAuthenticator(SlokedCrypto &crypto, SlokedCredentialProvider &provider, std::string salt, SlokedSocketEncryption *encryption)
        : crypto(crypto), provider(provider), salt(std::move(salt)), encryption(encryption) {}

    SlokedSlaveAuthenticator::~SlokedSlaveAuthenticator() {
        if (this->unwatchCredentials) {
            this->unwatchCredentials();
        }
    }

    bool SlokedSlaveAuthenticator::IsLoggedIn() const {
        return this->account.has_value();
    }

    std::optional<std::string> SlokedSlaveAuthenticator::GetAccount() const {
        return this->account;
    }

    std::string SlokedSlaveAuthenticator::InitiateLogin(const std::string keyId, Challenge ch) {
        this->account.reset();
        auto key = this->provider.GetByName(keyId).DeriveKey(this->salt);
        auto cipher = this->crypto.NewCipher(std::move(key));
        if (cipher->BlockSize() < sizeof(ch)) {
            throw SlokedError("Authentication: Authentication not supported for current cipher");
        }
        constexpr std::size_t NonceSize = sizeof(ch) / sizeof(uint8_t);
        union {
            uint8_t bytes[NonceSize];
            uint32_t value;
        } nonce;
        nonce.value = ch;
        std::vector<uint8_t> raw;
        raw.insert(raw.end(), cipher->BlockSize(), 0);
        for (std::size_t i = 0; i < NonceSize; i++) {
            raw[i] = nonce.bytes[i];
        }
        auto encrypted = cipher->Encrypt(raw);
        auto result = SlokedBase64::Encode(encrypted.data(), encrypted.data() + encrypted.size());
        return result;
    }

    void SlokedSlaveAuthenticator::FinalizeLogin(const std::string &keyId) {
        this->account = keyId;
        if (this->unwatchCredentials) {
            this->unwatchCredentials();
            this->unwatchCredentials = nullptr;
        }
        if (this->encryption) {
            auto key = this->provider.GetByName(keyId).DeriveKey(this->salt);
            auto cipher = this->crypto.NewCipher(std::move(key));
            this->encryption->SetEncryption(std::move(cipher));
            this->unwatchCredentials = this->provider.GetByName(keyId).Watch([this] {
                if (this->account.has_value()) {
                    auto key = this->provider.GetByName(this->account.value()).DeriveKey(this->salt);
                    auto cipher = this->crypto.NewCipher(std::move(key));
                    this->encryption->SetEncryption(std::move(cipher));
                }
            });
        }
    }

    SlokedAuthenticatorFactory::SlokedAuthenticatorFactory(SlokedCrypto &crypto, SlokedCredentialProvider &provider, std::string salt)
        : crypto(crypto), provider(provider), salt(std::move(salt)) {}

    std::unique_ptr<SlokedMasterAuthenticator> SlokedAuthenticatorFactory::NewMaster(SlokedSocketEncryption *encryption) {
        return std::make_unique<SlokedMasterAuthenticator>(this->crypto, this->provider, this->salt, encryption);
    }

    std::unique_ptr<SlokedSlaveAuthenticator> SlokedAuthenticatorFactory::NewSlave(SlokedSocketEncryption *encryption) {
        return std::make_unique<SlokedSlaveAuthenticator>(this->crypto, this->provider, this->salt, encryption);
    }
}