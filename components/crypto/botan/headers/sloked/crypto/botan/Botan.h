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

#ifndef SLOKED_THIRD_PARTY_CRYPTO_BOTAN_H_
#define SLOKED_THIRD_PARTY_CRYPTO_BOTAN_H_

#include <vector>

#include "sloked/core/Crypto.h"

namespace sloked {

    class SlokedBotanCrypto : public SlokedCrypto {
     public:
        class BotanKey : public Key {
         public:
            BotanKey(std::vector<uint8_t>);
            std::size_t Length() const final;
            std::unique_ptr<Key> Clone() const final;
            const Data &Get() const;

         private:
            Data key;
        };

        class BotanCipher : public Cipher {
         public:
            BotanCipher();
            ~BotanCipher();
            Data Encrypt(const Data &, const Key &, const Data &) final;
            Data Decrypt(const Data &, const Key &, const Data &) final;
            const CipherParameters &Parameters() const final;

            struct Impl;

         private:
            std::unique_ptr<Impl> impl;
        };

        class BotanRandom : public Random {
         public:
            BotanRandom();
            ~BotanRandom();
            uint8_t NextByte() final;

            struct Impl;

         private:
            std::unique_ptr<Impl> impl;
        };

        SlokedBotanCrypto();
        virtual ~SlokedBotanCrypto();
        const std::string &KDFName() const final;
        const CipherParameters &GetCipherParameters() const final;
        std::unique_ptr<Key> DeriveKey(std::size_t, const std::string &,
                                       const std::string &) final;
        std::unique_ptr<Cipher> NewCipher() final;
        std::unique_ptr<Random> NewRandom() final;

        static const EngineId Engine;

     private:
        struct Impl;

        static int engineId;
        std::unique_ptr<Impl> impl;
    };
}  // namespace sloked

#endif