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

#include "sloked/crypto/openssl/OpenSSL.h"

#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>

namespace sloked {

    static SlokedCrypto::CipherParameters CurrentCipherParameters{
        static_cast<std::size_t>(EVP_CIPHER_key_length(EVP_aes_256_cbc())),
        static_cast<std::size_t>(EVP_CIPHER_block_size(EVP_aes_256_cbc())),
        static_cast<std::size_t>(EVP_CIPHER_iv_length(EVP_aes_256_cbc())),
        "AES-256/CBC", "PKCS7"};

    int SlokedOpenSSLCrypto::engineId = 0;
    const SlokedCrypto::EngineId SlokedOpenSSLCrypto::Engine =
        reinterpret_cast<intptr_t>(
            std::addressof(SlokedOpenSSLCrypto::engineId));

    static void CheckErrors(int err, const std::string &msg) {
        if (err <= 0) {
            throw SlokedError("OpenSSLCrypto: Error " + std::to_string(err) +
                              " while " + msg);
        }
    }

    SlokedOpenSSLCrypto::OpenSSLKey::OpenSSLKey(std::vector<uint8_t> key)
        : Key(SlokedOpenSSLCrypto::Engine), key(std::move(key)) {}

    std::size_t SlokedOpenSSLCrypto::OpenSSLKey::Length() const {
        return this->key.size();
    }

    std::unique_ptr<SlokedCrypto::Key> SlokedOpenSSLCrypto::OpenSSLKey::Clone()
        const {
        return std::make_unique<OpenSSLKey>(this->key);
    }

    const std::vector<uint8_t> &SlokedOpenSSLCrypto::OpenSSLKey::Get() const {
        return this->key;
    }

    struct SlokedOpenSSLCrypto::OpenSSLCipher::Impl {
        EVP_CIPHER_CTX *cipher;
    };

    SlokedOpenSSLCrypto::OpenSSLCipher::OpenSSLCipher()
        : Cipher(SlokedOpenSSLCrypto::Engine),
          impl(std::make_unique<Impl>(Impl{nullptr})) {
        this->impl->cipher = EVP_CIPHER_CTX_new();
        if (this->impl->cipher == nullptr) {
            throw SlokedError("OpenSSLCrypto: Error creating cipher context");
        }
    }

    SlokedOpenSSLCrypto::OpenSSLCipher::~OpenSSLCipher() {
        EVP_CIPHER_CTX_free(this->impl->cipher);
    }

    SlokedCrypto::Data SlokedOpenSSLCrypto::OpenSSLCipher::Encrypt(
        const Data &data, const Key &key, const Data &ivdata) {
        std::unique_lock lock(this->mtx);
        auto cipher = this->impl->cipher;
        if (ivdata.size() != CurrentCipherParameters.IVSize) {
            throw SlokedError("OpenSSLCrypto: Invalid nonce");
        }
        const unsigned char *iv = ivdata.data();
        if (key.Engine != SlokedOpenSSLCrypto::Engine) {
            throw SlokedError("OpenSSLCrypto: Non-OpenSSL key");
        }
        const OpenSSLKey &openSSLKey = static_cast<const OpenSSLKey &>(key);
        CheckErrors(EVP_CipherInit_ex(cipher, EVP_aes_256_cbc(), nullptr,
                                      openSSLKey.Get().data(), iv, true),
                    "cipher initialization");
        CheckErrors(EVP_CIPHER_CTX_set_padding(cipher, true),
                    "cipher initialization");
        const int BlockSize = EVP_CIPHER_CTX_block_size(cipher);
        Data output;
        const std::size_t OutputSize =
            data.size() + (BlockSize - (data.size() % BlockSize));
        output.insert(output.end(), OutputSize, 0);
        int outsize, finsize;
        CheckErrors(EVP_CipherUpdate(cipher, output.data(), &outsize,
                                     data.data(), data.size()),
                    "encryption");
        CheckErrors(
            EVP_CipherFinal_ex(cipher, output.data() + outsize, &finsize),
            "cipher finalization");
        output.erase(output.begin() + outsize + finsize, output.end());
        return output;
    }

    SlokedCrypto::Data SlokedOpenSSLCrypto::OpenSSLCipher::Decrypt(
        const Data &data, const Key &key, const Data &ivdata) {
        std::unique_lock lock(this->mtx);
        auto cipher = this->impl->cipher;
        if (ivdata.size() != CurrentCipherParameters.IVSize) {
            throw SlokedError("OpenSSLCrypto: Invalid nonce");
        }
        const unsigned char *iv = ivdata.data();
        if (key.Engine != SlokedOpenSSLCrypto::Engine) {
            throw SlokedError("OpenSSLCrypto: Non-OpenSSL key");
        }
        const OpenSSLKey &openSSLKey = static_cast<const OpenSSLKey &>(key);
        CheckErrors(EVP_CipherInit_ex(cipher, EVP_aes_256_cbc(), nullptr,
                                      openSSLKey.Get().data(), iv, false),
                    "cipher initialization");
        CheckErrors(EVP_CIPHER_CTX_set_padding(cipher, true),
                    "cipher initialization");
        Data output;
        output.insert(output.end(), data.size(), 0);
        int outsize, finsize;
        CheckErrors(EVP_CipherUpdate(cipher, output.data(), &outsize,
                                     data.data(), data.size()),
                    "decryption");
        CheckErrors(
            EVP_CipherFinal_ex(cipher, output.data() + outsize, &finsize),
            "cipher finalization");
        output.erase(output.begin() + outsize + finsize, output.end());
        return output;
    }

    const SlokedCrypto::CipherParameters &
        SlokedOpenSSLCrypto::OpenSSLCipher::Parameters() const {
        return CurrentCipherParameters;
    }

    uint8_t SlokedOpenSSLCrypto::OpenSSLRandom::NextByte() {
        uint8_t value;
        if (RAND_bytes(&value, 1) != 1) {
            throw SlokedError("OpenSSLCrypto: Error getting random byte");
        }
        return value;
    }

    const std::string &SlokedOpenSSLCrypto::KDFName() const {
        static std::string KDF{"Scrypt"};
        return KDF;
    }

    const SlokedCrypto::CipherParameters &
        SlokedOpenSSLCrypto::GetCipherParameters() const {
        return CurrentCipherParameters;
    }

    std::unique_ptr<SlokedCrypto::Key> SlokedOpenSSLCrypto::DeriveKey(
        std::size_t key_length, const std::string &password,
        const std::string &salt) {
        std::vector<uint8_t> key;
        key.insert(key.end(), key_length, 0);
        EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_SCRYPT, NULL);
        CheckErrors(EVP_PKEY_derive_init(pctx), "key derivation init");
        CheckErrors(
            EVP_PKEY_CTX_set1_pbe_pass(pctx, password.c_str(), password.size()),
            "key derivation init");
        CheckErrors(
            EVP_PKEY_CTX_set1_scrypt_salt(pctx, salt.c_str(), salt.size()),
            "key derivation init");
        CheckErrors(EVP_PKEY_CTX_set_scrypt_N(pctx, 32768),
                    "key derivation init");
        CheckErrors(EVP_PKEY_CTX_set_scrypt_r(pctx, 8), "key derivation init");
        CheckErrors(EVP_PKEY_CTX_set_scrypt_p(pctx, 1), "key derivation init");
        std::size_t real_length = key_length;
        CheckErrors(EVP_PKEY_derive(pctx, key.data(), &real_length),
                    "key derivation");
        EVP_PKEY_CTX_free(pctx);
        return std::make_unique<OpenSSLKey>(std::move(key));
    }

    std::unique_ptr<SlokedCrypto::Cipher> SlokedOpenSSLCrypto::NewCipher() {
        return std::make_unique<OpenSSLCipher>();
    }

    std::unique_ptr<SlokedCrypto::Random> SlokedOpenSSLCrypto::NewRandom() {
        return std::make_unique<OpenSSLRandom>();
    }
}  // namespace sloked