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

#ifndef SLOKED_CORE_CRYPTO_H_
#define SLOKED_CORE_CRYPTO_H_

#include "sloked/core/Span.h"
#include <cinttypes>
#include <string>
#include <memory>
#include <vector>

namespace sloked {

    class SlokedCrypto {
     public:
        using EngineId = intptr_t;
        using Data = std::vector<uint8_t>;

        class Key {
         public:
            virtual ~Key() = default;
            virtual std::size_t Length() const = 0;

            const EngineId Engine;
         protected:
            Key(EngineId engine) : Engine(engine) {}
        };

        class Cipher {
         public:
            virtual ~Cipher() = default;
            virtual Data Encrypt(const Data &) = 0;
            virtual Data Decrypt(const Data &) = 0;
            virtual std::size_t BlockSize() const = 0;

            const EngineId Engine;
         protected:
            Cipher(EngineId engine) : Engine(engine) {}
        };

        virtual ~SlokedCrypto() = default;
        virtual std::unique_ptr<Key> DeriveKey(const std::string &, const std::string &) = 0;
        virtual std::unique_ptr<Cipher> NewCipher(const Key &) = 0;
    };
}

#endif