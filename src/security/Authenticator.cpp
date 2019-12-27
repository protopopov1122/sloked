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

    SlokedAuthenticator::SlokedAuthenticator(SlokedCrypto &crypto, SlokedAuthenticationProvider &provider, std::string salt)
        : crypto(crypto), random(crypto.NewRandom()), provider(provider), salt(std::move(salt)) {}

    SlokedAuthenticationProvider &SlokedAuthenticator::GetProvider() const {
        return this->provider;
    }

    SlokedCrypto &SlokedAuthenticator::GetCrypto() const {
        return this->crypto;
    }
    
    const std::string &SlokedAuthenticator::GetSalt() const {
        return this->salt;
    }

    SlokedAuthenticator::Challenge SlokedAuthenticator::GenerateChallenge() const {
        return this->random->NextInt<uint32_t>();
    }

    std::pair<std::string, std::unique_ptr<SlokedCrypto::Cipher>> SlokedAuthenticator::GenerateResponse(const std::string &keyId, Challenge ch) const {
        auto key = this->provider.GetByName(keyId).DeriveKey(this->GetSalt());
        auto cipher = this->crypto.NewCipher(std::move(key));
        auto response = this->GenerateResponse(*cipher, ch);
        return std::make_pair(std::move(response), std::move(cipher));
    }

    std::unique_ptr<SlokedCrypto::Cipher> SlokedAuthenticator::VerifyResponse(const std::string &keyId, Challenge ch, const std::string &actualResponse) const {
        auto key = this->provider.GetByName(keyId).DeriveKey(this->GetSalt());
        auto cipher = this->crypto.NewCipher(std::move(key));
        auto expected = this->GenerateResponse(*cipher, ch);
        if (expected == actualResponse) {
            return cipher;
        } else {
            return nullptr;
        }
    }

    std::string SlokedAuthenticator::GenerateResponse(SlokedCrypto::Cipher &cipher, Challenge ch) const {
        if (cipher.BlockSize() < sizeof(ch)) {
            throw SlokedError("Authentication: Authentication not supported for current cipher");
        }
        constexpr std::size_t NonceSize = sizeof(ch) / sizeof(uint8_t);
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
        auto result = SlokedBase64::Encode(encrypted.data(), encrypted.data() + encrypted.size());
        return result;
    }
}