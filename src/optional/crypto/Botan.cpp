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

#include "sloked/optional/crypto/Botan.h"
#include "sloked/core/Error.h"
#include <botan/pwdhash.h>
#include <botan/block_cipher.h>
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
            : cipher(Botan::BlockCipher::create("AES-256")) {
            if (this->cipher == nullptr) {
                throw SlokedError("BotanCrypto: Cipher AES-256 not found");
            } else {
                this->cipher->set_key(key.Get());
            }
        }

        std::unique_ptr<Botan::BlockCipher> cipher;
    };

    SlokedBotanCrypto::BotanCipher::BotanCipher(const BotanKey &key)
        : Cipher(SlokedBotanCrypto::Engine), impl(std::make_unique<Impl>(key)) {}

    SlokedBotanCrypto::BotanCipher::~BotanCipher() = default;

    SlokedCrypto::Data SlokedBotanCrypto::BotanCipher::Encrypt(const Data &input) {
        Data output = input;
        this->impl->cipher->encrypt(output);
        return output;
    }

    SlokedCrypto::Data SlokedBotanCrypto::BotanCipher::Decrypt(const Data &input) {
        Data output = input;
        this->impl->cipher->decrypt(output);
        return output;
    }

    std::size_t SlokedBotanCrypto::BotanCipher::BlockSize() const {
        return this->impl->cipher->block_size();
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