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

#ifndef SLOKED_NET_CRYPTOSOCKET_H_
#define SLOKED_NET_CRYPTOSOCKET_H_

#include "sloked/core/Crypto.h"
#include "sloked/net/Socket.h"

namespace sloked {

    class SlokedCryptoSocket : public SlokedSocket, public SlokedSocketEncryption {
     public:
        using CipherGenerator = std::function<std::unique_ptr<SlokedCrypto::Cipher>(const std::string &)>;

        SlokedCryptoSocket(std::unique_ptr<SlokedSocket>, std::unique_ptr<SlokedCrypto::Cipher>);
        SlokedCryptoSocket(const SlokedCryptoSocket &) = delete;
        SlokedCryptoSocket(SlokedCryptoSocket &&);
        SlokedCryptoSocket &operator=(const SlokedCryptoSocket &) = delete;
        SlokedCryptoSocket &operator=(SlokedCryptoSocket &&);

        bool Valid() final;
        void Close() final;
        std::size_t Available() final;
        bool Wait(std::chrono::system_clock::duration = std::chrono::system_clock::duration::zero()) final;
        std::optional<uint8_t> Read() final;
        std::vector<uint8_t> Read(std::size_t) final;
        void Write(SlokedSpan<const uint8_t>) final;
        void Write(uint8_t) final;
        std::unique_ptr<SlokedIOAwaitable> Awaitable() const final;
        SlokedSocketEncryption *GetEncryption() final;
        
        void SetEncryption(std::unique_ptr<SlokedCrypto::Cipher>) final;
        void RestoreDedaultEncryption() final;

     private:
        void Put(const uint8_t *, std::size_t);
        void Fetch(std::size_t = 0);

        std::unique_ptr<SlokedSocket> socket;
        std::unique_ptr<SlokedCrypto::Cipher> cipher;
        std::unique_ptr<SlokedCrypto::Cipher> defaultCipher;
        std::vector<uint8_t> encryptedBuffer;
        std::vector<uint8_t> buffer;
    };

    class SlokedCryptoServerSocket : public SlokedServerSocket {
     public:
        SlokedCryptoServerSocket(std::unique_ptr<SlokedServerSocket>, SlokedCrypto &, SlokedCrypto::Key &);
        SlokedCryptoServerSocket(const SlokedCryptoServerSocket &) = delete;
        SlokedCryptoServerSocket(SlokedCryptoServerSocket &&);
        SlokedCryptoServerSocket &operator=(const SlokedCryptoServerSocket &) = delete;
        SlokedCryptoServerSocket &operator=(SlokedCryptoServerSocket &&) = delete;

        bool Valid() final;
        void Start() final;
        void Close() final;
        std::unique_ptr<SlokedSocket> Accept(std::chrono::system_clock::duration = std::chrono::system_clock::duration::zero()) final;
        std::unique_ptr<SlokedIOAwaitable> Awaitable() const final;

     private:
        std::unique_ptr<SlokedServerSocket> serverSocket;
        SlokedCrypto &crypto;
        SlokedCrypto::Key &key;
    };

    class SlokedCryptoSocketFactory : public SlokedSocketFactory {
     public:
        SlokedCryptoSocketFactory(SlokedSocketFactory &, SlokedCrypto &, SlokedCrypto::Key &);
        SlokedCryptoSocketFactory(const SlokedCryptoSocketFactory &) = delete;
        SlokedCryptoSocketFactory(SlokedCryptoSocketFactory &&);

        SlokedCryptoSocketFactory &operator=(const SlokedCryptoSocketFactory &) = delete;
        SlokedCryptoSocketFactory &operator=(SlokedCryptoSocketFactory &&) = delete;

        std::unique_ptr<SlokedSocket> Connect(const std::string &, uint16_t) final;
        std::unique_ptr<SlokedServerSocket> Bind(const std::string &, uint16_t) final;

     private:
        SlokedSocketFactory &socketFactory;
        SlokedCrypto &crypto;
        SlokedCrypto::Key &key;
    };
}

#endif