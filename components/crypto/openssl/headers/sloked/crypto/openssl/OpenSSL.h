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

#ifndef SLOKED_CRYPTO_OPENSSL_OPENSSL_H_
#define SLOKED_CRYPTO_OPENSSL_OPENSSL_H_

#include <mutex>

#include "sloked/core/Crypto.h"

namespace sloked {

    class SlokedOpenSSLCrypto : public SlokedCrypto {
     public:
        class OpenSSLKey : public Key {
         public:
            OpenSSLKey(std::vector<uint8_t>);
            std::size_t Length() const final;
            const std::vector<uint8_t> &Get() const;

         private:
            std::vector<uint8_t> key;
        };

        class OpenSSLCipher : public Cipher {
         public:
            OpenSSLCipher(const OpenSSLKey &);
            virtual ~OpenSSLCipher();
            Data Encrypt(const Data &, const Data & = {}) final;
            Data Decrypt(const Data &, const Data & = {}) final;
            std::size_t BlockSize() const final;
            std::size_t IVSize() const final;

            struct Impl;

         private:
            std::unique_ptr<Impl> impl;
            mutable std::mutex mtx;
        };

        class OpenSSLOwningCipher : public OpenSSLCipher {
         public:
            OpenSSLOwningCipher(std::unique_ptr<OpenSSLKey>);

         private:
            std::unique_ptr<OpenSSLKey> key;
        };

        class OpenSSLRandom : public Random {
         public:
            uint8_t NextByte() final;
        };

        std::unique_ptr<Key> DeriveKey(const std::string &,
                                       const std::string &) final;
        std::unique_ptr<Cipher> NewCipher(const SlokedCrypto::Key &) final;
        std::unique_ptr<Cipher> NewCipher(
            std::unique_ptr<SlokedCrypto::Key>) final;
        std::unique_ptr<Random> NewRandom() final;

        static const EngineId Engine;

     private:
        static int engineId;
    };
}  // namespace sloked

#endif