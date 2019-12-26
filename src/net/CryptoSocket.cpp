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
#include "sloked/core/Hash.h"
#include <cassert>
#include <iostream>

namespace sloked {

    SlokedCryptoSocket::SlokedCryptoSocket(std::unique_ptr<SlokedSocket> socket, SlokedCrypto &crypto, std::unique_ptr<SlokedCrypto::Cipher> cipher, SlokedAuthenticationProvider *auth)
        : socket(std::move(socket)), crypto(std::ref(crypto)), cipher(std::move(cipher)), authenticationProvider(auth), authWatcher{nullptr}, accountId{} {}

    SlokedCryptoSocket::SlokedCryptoSocket(SlokedCryptoSocket &&socket)
        : socket(std::move(socket.socket)), crypto(std::move(socket.crypto)), cipher(std::move(socket.cipher)),
          authenticationProvider(std::move(socket.authenticationProvider)), authWatcher(std::move(socket.authWatcher)) {}
        
    SlokedCryptoSocket::~SlokedCryptoSocket() {
        if (this->authWatcher) {
            this->authWatcher();
        }
    }

    SlokedCryptoSocket &SlokedCryptoSocket::operator=(SlokedCryptoSocket &&socket) {
        this->socket = std::move(socket.socket);
        this->crypto = std::move(socket.crypto);
        this->cipher = std::move(socket.cipher);
        this->authenticationProvider = socket.authenticationProvider;
        socket.authenticationProvider = nullptr;
        this->authWatcher = std::move(socket.authWatcher);
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

    const SlokedSocketAuthentication *SlokedCryptoSocket::GetAuthentication() const {
        if (this->authenticationProvider) {
            return this;
        } else {
            return nullptr;
        }
    }

    SlokedSocketAuthentication *SlokedCryptoSocket::GetAuthentication() {
        if (this->authenticationProvider) {
            return this;
        } else {
            return nullptr;
        }
    }
    
    const std::string &SlokedCryptoSocket::GetAccount() const {
        return this->accountId;
    }

    void SlokedCryptoSocket::ChangeAccount(const std::string &id) {
        if (this->authenticationProvider && this->authenticationProvider->Has(id)) {
            auto newKey = this->authenticationProvider->GetByName(id).DeriveKey("");
            if (newKey) {
                if (this->authWatcher) {
                    this->authWatcher();
                }
                this->Fetch();
                this->Put(reinterpret_cast<const uint8_t *>(id.data()), id.size(), Command::Key);
                this->cipher = this->crypto.get().NewCipher(std::move(newKey));
                this->accountId = id;
                this->authWatcher = this->authenticationProvider->GetByName(id).Watch([this, id] {
                    auto newKey = this->authenticationProvider->GetByName(id).DeriveKey("");
                    if (newKey) {
                        this->cipher = this->crypto.get().NewCipher(std::move(newKey));
                    }
                });
            } else {
                throw SlokedError("CryptoSocket: Encryption key \'" + id + "\' deriving issue");
            }
        } else {
            throw SlokedError("CryptoSocket: Unknown encryption key \'" + id + "\'");
        }
    }

    template <typename T>
    constexpr uint8_t ByteAt(T value, std::size_t idx) {
        assert(idx < sizeof(T));
        return (value >> (idx << 3)) & 0xff;
    }

    void SlokedCryptoSocket::Put(const uint8_t *bytes, std::size_t length, Command cmd) {
        std::size_t totalLength = length;
        if (length % this->cipher->BlockSize() != 0) {
            totalLength = (length / this->cipher->BlockSize() + 1) * this->cipher->BlockSize();
        }
        std::vector<uint8_t> raw(bytes, bytes + length);
        raw.insert(raw.end(), totalLength - length, 0);
        auto crc32 = SlokedCrc32::Calculate(raw.begin(), raw.end());
        auto encrypted = this->cipher->Encrypt(raw);
        std::vector<uint8_t> header {
            static_cast<uint8_t>(cmd),
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
        constexpr std::size_t EncryptedHeaderSize = 9;
        do {
            auto chunk = this->socket->Read(this->socket->Available());
            this->encryptedBuffer.insert(this->encryptedBuffer.end(), chunk.begin(), chunk.end());
            if (this->encryptedBuffer.size() < EncryptedHeaderSize) {
                continue;
            }

            uint8_t command = this->encryptedBuffer.at(0);
            std::size_t length = this->encryptedBuffer.at(1) +
                (this->encryptedBuffer.at(2) << 8) +
                (this->encryptedBuffer.at(3) << 16) +
                (this->encryptedBuffer.at(4) << 24);
            uint32_t crc32 = this->encryptedBuffer.at(5) +
                (this->encryptedBuffer.at(6) << 8) +
                (this->encryptedBuffer.at(7) << 16) +
                (this->encryptedBuffer.at(8) << 24);
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
            switch (static_cast<Command>(command)) {
                case Command::Data:
                    this->buffer.insert(this->buffer.end(), raw.begin(), raw.begin() + length);
                    break;

                case Command::Key: {
                    std::string keyId{raw.begin(), raw.begin() + length};
                    if (this->authenticationProvider && this->authenticationProvider->Has(keyId)) {
                        auto newKey = this->authenticationProvider->GetByName(keyId).DeriveKey("");
                        if (newKey) {
                            if (this->authWatcher) {
                                this->authWatcher();
                            }
                            this->cipher = this->crypto.get().NewCipher(std::move(newKey));
                            this->accountId = keyId;
                            this->authWatcher = this->authenticationProvider->GetByName(keyId).Watch([this, keyId] {
                                auto newKey = this->authenticationProvider->GetByName(keyId).DeriveKey("");
                                if (newKey) {
                                    this->cipher = this->crypto.get().NewCipher(std::move(newKey));
                                }
                            });
                        } else {
                            throw SlokedError("CryptoSocket: Encryption key \'" + keyId + "\' deriving error");
                        }
                    } else {
                        throw SlokedError("CryptoSocket: Unknown encryption key \'" + keyId + "\'");
                    }
                } break;

                default:
                    break;
            }
        } while (this->buffer.size() < sz || this->socket->Available() > 0);
    }

    SlokedCryptoServerSocket::SlokedCryptoServerSocket(std::unique_ptr<SlokedServerSocket> serverSocket, SlokedCrypto &crypto, SlokedCrypto::Key &key, SlokedAuthenticationProvider *auth)
        : serverSocket(std::move(serverSocket)), crypto(crypto), key(key), authenticationProvider(auth) {}

    SlokedCryptoServerSocket::SlokedCryptoServerSocket(SlokedCryptoServerSocket &&serverSocket)
        : serverSocket(std::move(serverSocket.serverSocket)), crypto(serverSocket.crypto), key(serverSocket.key), authenticationProvider(std::move(serverSocket.authenticationProvider)) {}

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
                return std::make_unique<SlokedCryptoSocket>(std::move(rawSocket), this->crypto, this->crypto.NewCipher(this->key), this->authenticationProvider);
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

    SlokedCryptoSocketFactory::SlokedCryptoSocketFactory(SlokedSocketFactory &socketFactory, SlokedCrypto &crypto, SlokedCrypto::Key &key, SlokedAuthenticationProvider *auth)
        : socketFactory(socketFactory), crypto(crypto), key(key), authenticationProvider(auth) {}

    SlokedCryptoSocketFactory::SlokedCryptoSocketFactory(SlokedCryptoSocketFactory &&socketFactory)
        : socketFactory(socketFactory.socketFactory), crypto(socketFactory.crypto), key(socketFactory.key), authenticationProvider(std::move(socketFactory.authenticationProvider)) {}

    std::unique_ptr<SlokedSocket> SlokedCryptoSocketFactory::Connect(const std::string &host, uint16_t port) {
        auto rawSocket = this->socketFactory.Connect(host, port);
        if (rawSocket) {
            return std::make_unique<SlokedCryptoSocket>(std::move(rawSocket), this->crypto, this->crypto.NewCipher(this->key), this->authenticationProvider);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedServerSocket> SlokedCryptoSocketFactory::Bind(const std::string &host, uint16_t port) {
        auto rawSocket = this->socketFactory.Bind(host, port);
        if (rawSocket) {
            return std::make_unique<SlokedCryptoServerSocket>(std::move(rawSocket), this->crypto, this->key, this->authenticationProvider);
        } else {
            return nullptr;
        }
    }
}