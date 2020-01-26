/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#include "sloked/crypto/botan/Botan.h"
#include "sloked/core/Error.h"
#include <botan/pwdhash.h>
#include <botan/cipher_mode.h>
#include <botan/system_rng.h>

namespace sloked {

    int SlokedBotanCrypto::engineId = 0;
    const SlokedCrypto::EngineId SlokedBotanCrypto::Engine = reinterpret_cast<intptr_t>(std::addressof(SlokedBotanCrypto::engineId));

    SlokedBotanCrypto::BotanKey::BotanKey(std::vector<uint8_t> key)
        : Key(SlokedBotanCrypto::Engine), key(std::move(key)) {}

    std::size_t SlokedBotanCrypto::BotanKey::Length() const {
        return this->key.size();
    }

    const std::vector<uint8_t> &SlokedBotanCrypto::BotanKey::Get() const {
        return this->key;
    }

    struct SlokedBotanCrypto::BotanCipher::Impl {
        Impl(const BotanKey &key)
            : encryption(Botan::Cipher_Mode::create("AES-256/CBC/NoPadding", Botan::ENCRYPTION)),
              decryption(Botan::Cipher_Mode::create("AES-256/CBC/NoPadding", Botan::DECRYPTION)) {
            if (this->encryption == nullptr || this->decryption == nullptr) {
                throw SlokedError("BotanCrypto: Cipher AES-256/CBC not found");
            } else {
                this->encryption->set_key(key.Get());
                this->decryption->set_key(key.Get());
            }
        }

        std::unique_ptr<Botan::Cipher_Mode> encryption;
        std::unique_ptr<Botan::Cipher_Mode> decryption;
    };

    SlokedBotanCrypto::BotanCipher::BotanCipher(const BotanKey &key)
        : Cipher(SlokedBotanCrypto::Engine), impl(std::make_unique<Impl>(key)) {}

    SlokedBotanCrypto::BotanCipher::~BotanCipher() = default;

    SlokedCrypto::Data SlokedBotanCrypto::BotanCipher::Encrypt(const Data &input, const Data &iv) {
        Botan::secure_vector<uint8_t> output(input.begin(), input.end());
        if (iv.size() == this->impl->encryption->default_nonce_length()) {
            this->impl->encryption->start(iv);
        } else {
            std::vector<uint8_t> emptyNonce(this->impl->encryption->default_nonce_length(), 0);
            this->impl->encryption->start(emptyNonce);
        }
        this->impl->encryption->finish(output);
        this->impl->encryption->reset();
        return std::vector(output.begin(), output.end());
    }

    SlokedCrypto::Data SlokedBotanCrypto::BotanCipher::Decrypt(const Data &input, const Data &iv) {
        Botan::secure_vector<uint8_t> output(input.begin(), input.end());
        if (iv.size() == this->impl->decryption->default_nonce_length()) {
            this->impl->decryption->start(iv);
        } else {
            std::vector<uint8_t> emptyNonce(this->impl->encryption->default_nonce_length(), 0);
            this->impl->decryption->start(emptyNonce);
        }
        this->impl->decryption->finish(output);
        this->impl->decryption->reset();
        return std::vector(output.begin(), output.end());
    }

    std::size_t SlokedBotanCrypto::BotanCipher::BlockSize() const {
        constexpr std::size_t  AESBlockSize = 16;
        return AESBlockSize;
    }

    std::size_t SlokedBotanCrypto::BotanCipher::IVSize() const {
        return this->impl->encryption->default_nonce_length();
    }

    SlokedBotanCrypto::BotanOwningCipher::BotanOwningCipher(std::unique_ptr<BotanKey> key)
        : BotanCipher(*key), key(std::move(key)) {}

    uint8_t SlokedBotanCrypto::BotanRandom::NextByte() {
        return Botan::system_rng().next_byte();
    }

    struct SlokedBotanCrypto::Impl {
        Impl()
            : hashFamily(Botan::PasswordHashFamily::create("Scrypt")) {
            if (this->hashFamily == nullptr) {
                throw SlokedError("BotanCrypto: Hash family Scrypt not found");
            }
            this->hash = this->hashFamily->default_params();
        }

        std::unique_ptr<Botan::PasswordHashFamily> hashFamily;
        std::unique_ptr<Botan::PasswordHash> hash;
    };

    SlokedBotanCrypto::SlokedBotanCrypto()
        : impl(std::make_unique<Impl>()) {}

    SlokedBotanCrypto::~SlokedBotanCrypto() = default;

    std::unique_ptr<SlokedCrypto::Key> SlokedBotanCrypto::DeriveKey(const std::string &password, const std::string &salt) {
        constexpr std::size_t key_length = 32;
        std::vector<uint8_t> key;
        key.insert(key.end(), key_length, 0);
        this->impl->hash->derive_key(key.data(), key_length, password.c_str(), password.size(), reinterpret_cast<const uint8_t *>(salt.c_str()), salt.size());
        return std::make_unique<BotanKey>(std::move(key));
    }

    std::unique_ptr<SlokedCrypto::Cipher> SlokedBotanCrypto::NewCipher(const Key &key) {
        if (key.Engine != SlokedBotanCrypto::Engine) {
            throw SlokedError("BotanCrypto: Non-botan key");
        }
        const BotanKey &botanKey = static_cast<const BotanKey &>(key);
        return std::make_unique<BotanCipher>(botanKey);
    }

    std::unique_ptr<SlokedCrypto::Cipher> SlokedBotanCrypto::NewCipher(std::unique_ptr<Key> key) {
        if (key->Engine != SlokedBotanCrypto::Engine) {
            throw SlokedError("BotanCrypto: Non-botan key");
        }
        std::unique_ptr<BotanKey> botanKey{static_cast<BotanKey *>(key.release())};
        return std::make_unique<BotanOwningCipher>(std::move(botanKey));
    }

    std::unique_ptr<SlokedCrypto::Random> SlokedBotanCrypto::NewRandom() {
        return std::make_unique<BotanRandom>();
    }
}
