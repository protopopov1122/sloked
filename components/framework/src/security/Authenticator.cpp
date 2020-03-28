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

#include "sloked/security/Authenticator.h"

#include "sloked/core/Base64.h"

namespace sloked {

    SlokedBaseAuthenticator::SlokedBaseAuthenticator(
        SlokedCrypto &crypto, SlokedCredentialProvider &provider,
        std::string salt, SlokedSocketEncryption *encryption)
        : crypto(crypto), provider(provider), salt(std::move(salt)),
          encryption(encryption) {}

    SlokedBaseAuthenticator::~SlokedBaseAuthenticator() {
        if (this->unwatchCredentials) {
            this->unwatchCredentials();
        }
    }

    bool SlokedBaseAuthenticator::IsLoggedIn() const {
        return this->account.has_value();
    }

    std::string SlokedBaseAuthenticator::GetAccount() const {
        if (this->account.has_value()) {
            return this->account.value();
        } else {
            throw SlokedError("BaseAuthenticator: Not logged in");
        }
    }

    std::unique_ptr<SlokedCrypto::Cipher> SlokedBaseAuthenticator::DeriveCipher(
        const std::string &account) {
        if (auto acc = this->provider.GetByName(account).lock()) {
            auto key = acc->DeriveKey(this->salt);
            auto cipher = this->crypto.NewCipher(std::move(key));
            return cipher;
        } else {
            throw SlokedError("Authenticator: Account \'" + account +
                              "\' is not available");
        }
    }

    std::string SlokedBaseAuthenticator::GenerateToken(
        SlokedCrypto::Cipher &cipher, Challenge ch) {
        if (cipher.BlockSize() < sizeof(Challenge)) {
            throw SlokedError("Authenticator: Authentication not supported for "
                              "current cipher");
        }
        constexpr std::size_t NonceSize = sizeof(Challenge) / sizeof(uint8_t);
        union {
            uint8_t bytes[NonceSize];
            uint32_t value;
        } nonce;
        nonce.value = ch;
        std::vector<uint8_t> raw;
        raw.insert(raw.end(), cipher.BlockSize(), 0);
        for (std::size_t i = 0; i < NonceSize; i++) {
            raw[i] = nonce.bytes[i];
        }
        auto encrypted = cipher.Encrypt(raw);
        return SlokedBase64::Encode(encrypted.data(),
                                    encrypted.data() + encrypted.size());
    }

    void SlokedBaseAuthenticator::SetupEncryption() {
        if (!this->account.has_value()) {
            throw SlokedError("Authenticator: Log in required");
        }
        if (this->unwatchCredentials) {
            this->unwatchCredentials();
            this->unwatchCredentials = nullptr;
        }
        if (this->encryption) {
            auto cipher = this->DeriveCipher(this->account.value());
            this->encryption->SetEncryption(std::move(cipher));
            if (auto acc =
                    this->provider.GetByName(this->account.value()).lock()) {
                this->unwatchCredentials = acc->Watch([this] {
                    if (this->account.has_value()) {
                        auto cipher = this->DeriveCipher(this->account.value());
                        this->encryption->SetEncryption(std::move(cipher));
                    }
                });
            } else {
                throw SlokedError("Authenticator: Account \'" +
                                  this->account.value() +
                                  "\' is not available");
            }
        }
    }

    SlokedMasterAuthenticator::SlokedMasterAuthenticator(
        SlokedCrypto &crypto, SlokedCredentialProvider &provider,
        std::string salt, SlokedSocketEncryption *encryption)
        : SlokedBaseAuthenticator(crypto, provider, std::move(salt),
                                  encryption),
          random(crypto.NewRandom()) {}

    SlokedMasterAuthenticator::~SlokedMasterAuthenticator() {
        if (this->unwatchCredentials) {
            this->unwatchCredentials();
        }
    }

    SlokedMasterAuthenticator::Challenge
        SlokedMasterAuthenticator::InitiateLogin() {
        this->nonce = this->random->NextInt<Challenge>();
        this->account.reset();
        return this->nonce.value();
    }

    bool SlokedMasterAuthenticator::ContinueLogin(const std::string &account,
                                                  const std::string &token) {
        if (!this->nonce.has_value()) {
            throw SlokedError("AuthenticationMaster: Initiate login first");
        }
        auto cipher = this->DeriveCipher(account);
        auto expectedToken = this->GenerateToken(*cipher, this->nonce.value());
        this->nonce.reset();
        if (expectedToken == token) {
            this->account = account;
            return true;
        } else {
            return false;
        }
    }

    void SlokedMasterAuthenticator::FinalizeLogin() {
        this->SetupEncryption();
    }

    SlokedSlaveAuthenticator::SlokedSlaveAuthenticator(
        SlokedCrypto &crypto, SlokedCredentialProvider &provider,
        std::string salt, SlokedSocketEncryption *encryption)
        : SlokedBaseAuthenticator(crypto, provider, std::move(salt),
                                  encryption) {}

    SlokedSlaveAuthenticator::~SlokedSlaveAuthenticator() {
        if (this->unwatchCredentials) {
            this->unwatchCredentials();
        }
    }

    std::string SlokedSlaveAuthenticator::InitiateLogin(const std::string keyId,
                                                        Challenge ch) {
        this->account.reset();
        auto cipher = this->DeriveCipher(keyId);
        return this->GenerateToken(*cipher, ch);
    }

    void SlokedSlaveAuthenticator::ContinueLogin(const std::string &keyId) {
        if (this->account.has_value()) {
            throw SlokedError("SlaveAuthenticator: Login is not initiated");
        }
        this->account = keyId;
    }

    void SlokedSlaveAuthenticator::FinalizeLogin() {
        this->SetupEncryption();
    }

    SlokedAuthenticatorFactory::SlokedAuthenticatorFactory(
        SlokedCrypto &crypto, SlokedCredentialProvider &provider,
        std::string salt)
        : crypto(crypto), provider(provider), salt(std::move(salt)) {}

    std::unique_ptr<SlokedMasterAuthenticator>
        SlokedAuthenticatorFactory::NewMaster(
            SlokedSocketEncryption *encryption) {
        return std::make_unique<SlokedMasterAuthenticator>(
            this->crypto, this->provider, this->salt, encryption);
    }

    std::unique_ptr<SlokedSlaveAuthenticator>
        SlokedAuthenticatorFactory::NewSlave(
            SlokedSocketEncryption *encryption) {
        return std::make_unique<SlokedSlaveAuthenticator>(
            this->crypto, this->provider, this->salt, encryption);
    }
}  // namespace sloked