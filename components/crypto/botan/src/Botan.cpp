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

#include "sloked/crypto/botan/Botan.h"

#include <botan/cipher_mode.h>
#include <botan/pwdhash.h>
#include <botan/system_rng.h>

#include "sloked/core/Error.h"

namespace sloked {

    int SlokedBotanCrypto::engineId = 0;
    const SlokedCrypto::EngineId SlokedBotanCrypto::Engine =
        reinterpret_cast<intptr_t>(std::addressof(SlokedBotanCrypto::engineId));

    static SlokedCrypto::CipherParameters CurrentCipherParameters{
        32, 16, 16, "AES-256/CBC", {}};

    SlokedBotanCrypto::BotanKey::BotanKey(std::vector<uint8_t> key)
        : Key(SlokedBotanCrypto::Engine), key(std::move(key)) {}

    std::size_t SlokedBotanCrypto::BotanKey::Length() const {
        return this->key.size();
    }

    std::unique_ptr<SlokedCrypto::Key> SlokedBotanCrypto::BotanKey::Clone()
        const {
        return std::make_unique<BotanKey>(this->key);
    }

    const std::vector<uint8_t> &SlokedBotanCrypto::BotanKey::Get() const {
        return this->key;
    }

    struct SlokedBotanCrypto::BotanCipher::Impl {
        Impl()
            : encryption(Botan::Cipher_Mode::create("AES-256/CBC/NoPadding",
                                                    Botan::ENCRYPTION)),
              decryption(Botan::Cipher_Mode::create("AES-256/CBC/NoPadding",
                                                    Botan::DECRYPTION)) {
            if (this->encryption == nullptr || this->decryption == nullptr) {
                throw SlokedError("BotanCrypto: Cipher AES-256/CBC not found");
            }
        }

        std::unique_ptr<Botan::Cipher_Mode> encryption;
        std::unique_ptr<Botan::Cipher_Mode> decryption;
    };

    SlokedBotanCrypto::BotanCipher::BotanCipher()
        : Cipher(SlokedBotanCrypto::Engine), impl(std::make_unique<Impl>()) {}

    SlokedBotanCrypto::BotanCipher::~BotanCipher() = default;

    SlokedCrypto::Data SlokedBotanCrypto::BotanCipher::Encrypt(
        const Data &input, const Key &key, const Data &iv) {
        Botan::secure_vector<uint8_t> output(input.begin(), input.end());
        if (key.Engine != SlokedBotanCrypto::Engine) {
            throw SlokedError("BotanCrypto: Non-botan key");
        }
        const BotanKey &botanKey = static_cast<const BotanKey &>(key);
        this->impl->encryption->set_key(botanKey.Get());
        this->impl->encryption->start(iv);
        this->impl->encryption->finish(output);
        this->impl->encryption->reset();
        return std::vector(output.begin(), output.end());
    }

    SlokedCrypto::Data SlokedBotanCrypto::BotanCipher::Decrypt(
        const Data &input, const Key &key, const Data &iv) {
        Botan::secure_vector<uint8_t> output(input.begin(), input.end());
        if (key.Engine != SlokedBotanCrypto::Engine) {
            throw SlokedError("BotanCrypto: Non-botan key");
        }
        const BotanKey &botanKey = static_cast<const BotanKey &>(key);
        this->impl->decryption->set_key(botanKey.Get());
        this->impl->decryption->start(iv);
        this->impl->decryption->finish(output);
        this->impl->decryption->reset();
        return std::vector(output.begin(), output.end());
    }

    const SlokedCrypto::CipherParameters &
        SlokedBotanCrypto::BotanCipher::Parameters() const {
        return CurrentCipherParameters;
    }

    struct SlokedBotanCrypto::BotanRandom::Impl {
        std::unique_ptr<Botan::RandomNumberGenerator> rng;
    };

    SlokedBotanCrypto::BotanRandom::BotanRandom()
        : impl(std::make_unique<Impl>()) {
#if defined(BOTAN_HAS_SYSTEM_RNG)
        this->impl->rng = std::make_unique<Botan::System_RNG>();
#else
        this->impl->rng = std::make_unique<Botan::AutoSeeded_RNG>();
#endif
    }

    SlokedBotanCrypto::BotanRandom::~BotanRandom() = default;

    uint8_t SlokedBotanCrypto::BotanRandom::NextByte() {
        return this->impl->rng->next_byte();
    }

    struct SlokedBotanCrypto::Impl {
        Impl() : hashFamily(Botan::PasswordHashFamily::create("Scrypt")) {
            if (this->hashFamily == nullptr) {
                throw SlokedError("BotanCrypto: Hash family Scrypt not found");
            }
            this->hash = this->hashFamily->default_params();
        }

        std::unique_ptr<Botan::PasswordHashFamily> hashFamily;
        std::unique_ptr<Botan::PasswordHash> hash;
    };

    SlokedBotanCrypto::SlokedBotanCrypto() : impl(std::make_unique<Impl>()) {}

    SlokedBotanCrypto::~SlokedBotanCrypto() = default;

    const std::string &SlokedBotanCrypto::KDFName() const {
        static std::string KDF{"Scrypt"};
        return KDF;
    }

    const SlokedCrypto::CipherParameters &
        SlokedBotanCrypto::GetCipherParameters() const {
        return CurrentCipherParameters;
    }

    std::unique_ptr<SlokedCrypto::Key> SlokedBotanCrypto::DeriveKey(
        std::size_t key_length, const std::string &password,
        const std::string &salt) {
        std::vector<uint8_t> key;
        key.insert(key.end(), key_length, 0);
        this->impl->hash->derive_key(
            key.data(), key_length, password.c_str(), password.size(),
            reinterpret_cast<const uint8_t *>(salt.c_str()), salt.size());
        return std::make_unique<BotanKey>(std::move(key));
    }

    std::unique_ptr<SlokedCrypto::Cipher> SlokedBotanCrypto::NewCipher() {
        return std::make_unique<BotanCipher>();
    }

    std::unique_ptr<SlokedCrypto::Random> SlokedBotanCrypto::NewRandom() {
        return std::make_unique<BotanRandom>();
    }
}  // namespace sloked
