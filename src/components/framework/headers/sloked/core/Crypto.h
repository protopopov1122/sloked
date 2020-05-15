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

#ifndef SLOKED_CORE_CRYPTO_H_
#define SLOKED_CORE_CRYPTO_H_

#include <cinttypes>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>
#include <limits>

#include "sloked/core/Span.h"

namespace sloked {

    class SlokedCrypto {
     public:
        using EngineId = intptr_t;
        using Data = std::vector<uint8_t>;

        class Key {
         public:
            virtual ~Key() = default;
            virtual std::size_t Length() const = 0;
            virtual std::unique_ptr<Key> Clone() const = 0;

            const EngineId Engine;

         protected:
            Key(EngineId engine) : Engine(engine) {}
        };

        struct CipherParameters {
            std::size_t KeySize;
            std::size_t BlockSize;
            std::size_t IVSize;
            std::string algorithm;
            std::string Padding;
        };

        class Cipher {
         public:
            virtual ~Cipher() = default;
            virtual Data Encrypt(const Data &, const Key &, const Data &) = 0;
            virtual Data Decrypt(const Data &, const Key &, const Data &) = 0;
            virtual const CipherParameters &Parameters() const = 0;

            const EngineId Engine;

         protected:
            Cipher(EngineId engine) : Engine(engine) {}
        };

        class Random {
         public:
            virtual ~Random() = default;
            virtual uint8_t NextByte() = 0;

            template <typename T = uint32_t>
            typename std::enable_if_t<std::is_integral_v<T>, T> NextInt() {
                constexpr std::size_t Size = sizeof(T) / sizeof(uint8_t);
                T value{0};
                for (std::size_t i = 0; i < Size; i++) {
                    value <<= std::numeric_limits<uint8_t>::digits;
                    value |= this->NextByte();
                }
                return value;
            }
        };

        virtual ~SlokedCrypto() = default;
        virtual const std::string &KDFName() const = 0;
        virtual const CipherParameters &GetCipherParameters() const = 0;
        virtual std::unique_ptr<Key> DeriveKey(std::size_t, const std::string &,
                                               const std::string &) = 0;
        virtual std::unique_ptr<Cipher> NewCipher() = 0;
        virtual std::unique_ptr<Random> NewRandom() = 0;
    };
}  // namespace sloked

#endif