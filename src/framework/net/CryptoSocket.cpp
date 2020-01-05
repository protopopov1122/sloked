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

#include "sloked/net/CryptoSocket.h"
#include "sloked/core/Error.h"
#include "sloked/core/Hash.h"
#include <cassert>
#include <iostream>

namespace sloked {

    SlokedCryptoSocket::SlokedCryptoSocket(std::unique_ptr<SlokedSocket> socket, std::unique_ptr<SlokedCrypto::Cipher> cipher)
        : socket(std::move(socket)), cipher(std::move(cipher)) {}

    SlokedCryptoSocket::SlokedCryptoSocket(SlokedCryptoSocket &&socket)
        : socket(std::move(socket.socket)), cipher(std::move(socket.cipher)), defaultCipher(std::move(socket.defaultCipher)),
          encryptedBuffer(std::move(socket.encryptedBuffer)), buffer(std::move(socket.buffer)) {}

    SlokedCryptoSocket &SlokedCryptoSocket::operator=(SlokedCryptoSocket &&socket) {
        this->socket = std::move(socket.socket);
        this->cipher = std::move(socket.cipher);
        this->defaultCipher = std::move(socket.defaultCipher);
        this->encryptedBuffer = std::move(socket.encryptedBuffer);
        this->buffer = std::move(socket.buffer);
        return *this;
    }

    bool SlokedCryptoSocket::Valid() {
        return this->socket != nullptr && this->socket->Valid();
    }

    void SlokedCryptoSocket::Close() {
        if (this->socket) {
            this->socket->Close();
            this->socket = nullptr;
            this->cipher = nullptr;
        }
    }

    std::size_t SlokedCryptoSocket::Available() {
        if (this->Valid()) {
            this->Fetch();
            return this->buffer.size();
        } else {
            throw SlokedError("CryptoSocket: Invalid socket");
        }
    }

    bool SlokedCryptoSocket::Wait(std::chrono::system_clock::duration timeout) {
        if (this->Valid()) {
            if (this->buffer.empty()) {
                this->socket->Wait(timeout);
            }
            this->Fetch();
            return this->buffer.size();
        } else {
            return false;
        }
    }
    
    std::optional<uint8_t> SlokedCryptoSocket::Read() {
        if (this->Valid()) {
            this->Fetch();
            if (!this->buffer.empty()) {
                auto res = this->buffer.at(0);
                this->buffer.erase(this->buffer.begin());
                return res;
            } else {
                return {};
            }
        } else {
            throw SlokedError("CryptoSocket: Invalid socket");
        }
    }

    std::vector<uint8_t> SlokedCryptoSocket::Read(std::size_t count) {
        if (this->Valid()) {
            this->Fetch(count);
            std::vector<uint8_t> msg(this->buffer.begin(), this->buffer.begin() + count);
            this->buffer.erase(this->buffer.begin(), this->buffer.begin() + count);
            return msg;
        } else {
            throw SlokedError("CryptoSocket: Invalid socket");
        }
    }
    
    void SlokedCryptoSocket::Write(SlokedSpan<const uint8_t> data) {
        if (this->Valid()) {
            this->Put(data.Data(), data.Size());
        } else {
            throw SlokedError("CryptoSocket: Invalid socket");
        }
    }

    void SlokedCryptoSocket::Write(uint8_t data) {
        if (this->Valid()) {
            this->Put(&data, 1);
        } else {
            throw SlokedError("CryptoSocket: Invalid socket");
        }
    }

    std::unique_ptr<SlokedIOAwaitable> SlokedCryptoSocket::Awaitable() const {
        return this->socket->Awaitable();
    }

    SlokedSocketEncryption *SlokedCryptoSocket::GetEncryption() {
        return this;
    }

    void SlokedCryptoSocket::SetEncryption(std::unique_ptr<SlokedCrypto::Cipher> cipher) {
        if (this->defaultCipher == nullptr) {
            this->defaultCipher = std::move(this->cipher);
        }
        this->cipher = std::move(cipher);
    }

    void SlokedCryptoSocket::RestoreDedaultEncryption() {
        if (this->defaultCipher) {
            this->cipher = std::move(this->defaultCipher);
        }
    }

    template <typename T>
    constexpr uint8_t ByteAt(T value, std::size_t idx) {
        assert(idx < sizeof(T));
        return (value >> (idx << 3)) & 0xff;
    }

    void SlokedCryptoSocket::Put(const uint8_t *bytes, std::size_t length) {
        std::size_t totalLength = length;
        if (length % this->cipher->BlockSize() != 0) {
            totalLength = (length / this->cipher->BlockSize() + 1) * this->cipher->BlockSize();
        }
        std::vector<uint8_t> raw(bytes, bytes + length);
        raw.insert(raw.end(), totalLength - length, 0);
        auto crc32 = SlokedCrc32::Calculate(raw.begin(), raw.end());
        auto encrypted = this->cipher->Encrypt(raw);
        std::vector<uint8_t> header {
            ByteAt(length, 0),
            ByteAt(length, 1),
            ByteAt(length, 2),
            ByteAt(length, 3),
            ByteAt(crc32, 0),
            ByteAt(crc32, 1),
            ByteAt(crc32, 2),
            ByteAt(crc32, 3)
        };
        encrypted.insert(encrypted.begin(), header.begin(), header.end());
        this->socket->Write(SlokedSpan(encrypted.data(), encrypted.size()));
    }

    void SlokedCryptoSocket::Fetch(std::size_t sz) {
        constexpr std::size_t EncryptedHeaderSize = 8;
        do {
            auto chunk = this->socket->Read(this->socket->Available());
            this->encryptedBuffer.insert(this->encryptedBuffer.end(), chunk.begin(), chunk.end());
            if (this->encryptedBuffer.size() < EncryptedHeaderSize) {
                continue;
            }

            std::size_t length = this->encryptedBuffer.at(0) +
                (this->encryptedBuffer.at(1) << 8) +
                (this->encryptedBuffer.at(2) << 16) +
                (this->encryptedBuffer.at(3) << 24);
            uint32_t crc32 = this->encryptedBuffer.at(4) +
                (this->encryptedBuffer.at(5) << 8) +
                (this->encryptedBuffer.at(6) << 16) +
                (this->encryptedBuffer.at(7) << 24);
            std::size_t totalLength = length;
            if (length % this->cipher->BlockSize() != 0) {
                totalLength = (length / this->cipher->BlockSize() + 1) * this->cipher->BlockSize();
            }
            if (this->encryptedBuffer.size() < totalLength + EncryptedHeaderSize) {
                continue;
            }

            std::vector<uint8_t> encrypted(this->encryptedBuffer.begin() + EncryptedHeaderSize, this->encryptedBuffer.begin() + EncryptedHeaderSize + totalLength);
            this->encryptedBuffer.erase(this->encryptedBuffer.begin(), this->encryptedBuffer.begin() + EncryptedHeaderSize + totalLength);
            auto raw = this->cipher->Decrypt(encrypted);
            auto actualCrc32 = SlokedCrc32::Calculate(raw.begin(), raw.end());
            if (actualCrc32 != crc32) {
                throw SlokedError("CryptoSocket: Actual CRC32 doesn't equal to expected");
            }
            this->buffer.insert(this->buffer.end(), raw.begin(), raw.begin() + length);
        } while (this->buffer.size() < sz || this->socket->Available() > 0);
    }

    SlokedCryptoServerSocket::SlokedCryptoServerSocket(std::unique_ptr<SlokedServerSocket> serverSocket, SlokedCrypto &crypto, SlokedCrypto::Key &key)
        : serverSocket(std::move(serverSocket)), crypto(crypto), key(key) {}

    SlokedCryptoServerSocket::SlokedCryptoServerSocket(SlokedCryptoServerSocket &&serverSocket)
        : serverSocket(std::move(serverSocket.serverSocket)), crypto(serverSocket.crypto), key(serverSocket.key) {}

    bool SlokedCryptoServerSocket::Valid() {
        return this->serverSocket != nullptr && this->serverSocket->Valid();
    }

    void SlokedCryptoServerSocket::Start() {
        if (this->Valid()) {
            this->serverSocket->Start();
        } else {
            throw SlokedError("CryptoServerSocket: Invalid socket");
        }
    }

    void SlokedCryptoServerSocket::Close() {
        if (this->serverSocket) {
            this->serverSocket->Close();
            this->serverSocket = nullptr;
        }
    }

    std::unique_ptr<SlokedSocket> SlokedCryptoServerSocket::Accept(std::chrono::system_clock::duration timeout) {
        if (this->Valid()) {
            auto rawSocket = this->serverSocket->Accept(timeout);
            if (rawSocket) {
                return std::make_unique<SlokedCryptoSocket>(std::move(rawSocket), this->crypto.NewCipher(this->key));
            } else {
                return nullptr;
            }
        } else {
            throw SlokedError("CryptoServerSocket: Invalid socket");
        }
    }

    std::unique_ptr<SlokedIOAwaitable> SlokedCryptoServerSocket::Awaitable() const {
        return this->serverSocket->Awaitable();
    }

    SlokedCryptoSocketFactory::SlokedCryptoSocketFactory(SlokedSocketFactory &socketFactory, SlokedCrypto &crypto, SlokedCrypto::Key &key)
        : socketFactory(socketFactory), crypto(crypto), key(key) {}

    SlokedCryptoSocketFactory::SlokedCryptoSocketFactory(SlokedCryptoSocketFactory &&socketFactory)
        : socketFactory(socketFactory.socketFactory), crypto(socketFactory.crypto), key(socketFactory.key) {}

    std::unique_ptr<SlokedSocket> SlokedCryptoSocketFactory::Connect(const std::string &host, uint16_t port) {
        auto rawSocket = this->socketFactory.Connect(host, port);
        if (rawSocket) {
            return std::make_unique<SlokedCryptoSocket>(std::move(rawSocket), this->crypto.NewCipher(this->key));
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedServerSocket> SlokedCryptoSocketFactory::Bind(const std::string &host, uint16_t port) {
        auto rawSocket = this->socketFactory.Bind(host, port);
        if (rawSocket) {
            return std::make_unique<SlokedCryptoServerSocket>(std::move(rawSocket), this->crypto, this->key);
        } else {
            return nullptr;
        }
    }
}