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

#include "sloked/net/CryptoSocket.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedCryptoSocket::SlokedCryptoSocket(std::unique_ptr<SlokedSocket> socket, std::unique_ptr<SlokedCrypto::Cipher> cipher)
        : socket(std::move(socket)), cipher(std::move(cipher)) {}

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

    bool SlokedCryptoSocket::Wait(long timeout) {
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

    void SlokedCryptoSocket::Put(const uint8_t *bytes, std::size_t length) {
        std::size_t totalLength = length;
        if (length % this->cipher->BlockSize() != 0) {
            totalLength = (length / this->cipher->BlockSize() + 1) * this->cipher->BlockSize();
        }
        std::vector<uint8_t> raw(bytes, bytes + length);
        raw.insert(raw.end(), totalLength - length, 0);
        auto encrypted = this->cipher->Encrypt(raw);
        std::vector<uint8_t> header {
            static_cast<uint8_t>(length & 0xff),
            static_cast<uint8_t>((length >> 8) & 0xff),
            static_cast<uint8_t>((length >> 16) & 0xff),
            static_cast<uint8_t>((length >> 24) & 0xff)
        };
        encrypted.insert(encrypted.begin(), header.begin(), header.end());
        this->socket->Write(SlokedSpan(encrypted.data(), encrypted.size()));
    }

    void SlokedCryptoSocket::Fetch(std::size_t sz) {
        constexpr std::size_t EncryptedHeaderSize = 4;
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
            this->buffer.insert(this->buffer.end(), raw.begin(), raw.begin() + length);
        } while (this->buffer.size() < sz || this->socket->Available() > 0);
    }

    SlokedCryptoServerSocket::SlokedCryptoServerSocket(std::unique_ptr<SlokedServerSocket> serverSocket, SlokedCrypto &crypto, SlokedCrypto::Key &key)
        : serverSocket(std::move(serverSocket)), crypto(crypto), key(key) {}

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

    std::unique_ptr<SlokedSocket> SlokedCryptoServerSocket::Accept(long timeout) {
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