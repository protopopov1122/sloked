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
        SlokedCrypto &crypto, SlokedCredentialStorage &provider,
        std::string salt, SlokedSocketEncryption *encryption)
        : crypto(crypto), provider(provider), salt(std::move(salt)),
          encryption(encryption) {
        if (this->encryption) {
            this->initialKey = this->encryption->GetEncryptionKey();
        }
    }

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

    void SlokedBaseAuthenticator::Logout() {
        if (this->unwatchCredentials) {
            this->unwatchCredentials();
            this->unwatchCredentials = nullptr;
        }
        this->account.reset();
        if (this->encryption) {
            if (this->initialKey) {
                this->encryption->SetEncryptionKey(this->initialKey->Clone(),
                                                   "");
            } else {
                this->encryption->SetEncryptionKey(nullptr, "");
            }
        }
    }

    std::unique_ptr<SlokedCrypto::Key> SlokedBaseAuthenticator::DeriveKey(
        std::size_t keyLength, const std::string &account) {
        if (auto acc = this->provider.GetByName(account)) {
            return acc->DeriveKey(keyLength, this->salt);
        } else {
            throw SlokedError("Authenticator: Account \'" + account +
                              "\' is not available");
        }
    }

    std::string SlokedBaseAuthenticator::GenerateToken(
        SlokedCrypto::Cipher &cipher, SlokedCrypto::Key &key, Challenge ch) {
        if (cipher.Parameters().BlockSize < sizeof(Challenge)) {
            throw SlokedError("Authenticator: Authentication not supported for "
                              "current cipher");
        }
        constexpr std::size_t NonceSize = sizeof(Challenge) / sizeof(uint8_t);
        std::vector<uint8_t> raw;
        for (std::size_t i = 0; i < NonceSize; i++) {
            raw.push_back((ch >> (i << 3)) & 0xff);
        }
        SlokedCrypto::Data iv(cipher.Parameters().IVSize, 0);
        auto encrypted = cipher.Encrypt(raw, key, iv);
        return SlokedBase64::Encode(encrypted.data(),
                                    encrypted.data() + encrypted.size());
    }

    void SlokedBaseAuthenticator::SetupEncryption(bool sendNotification) {
        if (!this->account.has_value()) {
            throw SlokedError("Authenticator: Log in required");
        }
        if (this->unwatchCredentials) {
            this->unwatchCredentials();
            this->unwatchCredentials = nullptr;
        }
        if (this->encryption) {
            auto key =
                this->DeriveKey(this->crypto.GetCipherParameters().KeySize,
                                this->account.value());
            this->encryption->SetEncryptionKey(
                std::move(key), sendNotification
                                    ? this->account
                                    : std::optional<std::string>{});
            if (auto acc = this->provider.GetByName(this->account.value())) {
                this->unwatchCredentials = acc->Watch(
                    [this, sendNotification,
                     keySize = this->crypto.GetCipherParameters().KeySize] {
                        if (this->account.has_value()) {
                            auto key =
                                this->DeriveKey(keySize, this->account.value());
                            this->encryption->SetEncryptionKey(
                                std::move(key),
                                sendNotification
                                    ? this->account
                                    : std::optional<std::string>{});
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
        SlokedCrypto &crypto, SlokedCredentialStorage &provider,
        std::string salt, SlokedSocketEncryption *encryption)
        : SlokedBaseAuthenticator(crypto, provider, std::move(salt),
                                  encryption),
          cipher(crypto.NewCipher()), random(crypto.NewRandom()) {}

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
        auto key = this->DeriveKey(this->cipher->Parameters().KeySize, account);
        auto expectedToken =
            this->GenerateToken(*this->cipher, *key, this->nonce.value());
        this->nonce.reset();
        if (expectedToken == token) {
            this->account = account;
            return true;
        } else {
            return false;
        }
    }

    void SlokedMasterAuthenticator::FinalizeLogin() {
        this->SetupEncryption(true);
    }

    SlokedSlaveAuthenticator::SlokedSlaveAuthenticator(
        SlokedCrypto &crypto, SlokedCredentialStorage &provider,
        std::string salt, SlokedSocketEncryption *encryption)
        : SlokedBaseAuthenticator(crypto, provider, std::move(salt),
                                  encryption),
          cipher(crypto.NewCipher()),
          unbindEncryptionListener{nullptr}, pending{} {

        if (this->encryption) {
            this->unbindEncryptionListener =
                this->encryption->OnKeyChange([this](auto &keyId) {
                    if (keyId == this->pending) {
                        if (keyId.has_value()) {
                            this->account = std::move(this->pending);
                            this->pending.reset();
                            this->SetupEncryption(false);
                        } else {
                            this->pending.reset();
                            this->Logout();
                        }
                    } else {
                        this->pending.reset();
                        throw SlokedError("SlaveAuthenticator: Unexpected "
                                          "encryption key change");
                    }
                });
        }
    }

    SlokedSlaveAuthenticator::~SlokedSlaveAuthenticator() {
        this->Close();
    }

    std::string SlokedSlaveAuthenticator::InitiateLogin(const std::string keyId,
                                                        Challenge ch) {
        this->account.reset();
        this->pending = keyId;
        auto key = this->DeriveKey(this->cipher->Parameters().KeySize, keyId);
        return this->GenerateToken(*this->cipher, *key, ch);
    }

    void SlokedSlaveAuthenticator::Close() {
        if (this->unbindEncryptionListener) {
            this->unbindEncryptionListener();
            this->unbindEncryptionListener = nullptr;
        }
        if (this->unwatchCredentials) {
            this->unwatchCredentials();
            this->unwatchCredentials = nullptr;
        }
    }

    SlokedAuthenticatorFactory::SlokedAuthenticatorFactory(
        SlokedCrypto &crypto, SlokedCredentialStorage &provider,
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