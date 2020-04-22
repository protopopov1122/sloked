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

#include "sloked/net/CryptoSocket.h"

#include <cassert>
#include <iostream>

#include "sloked/core/Encoding.h"
#include "sloked/core/Error.h"
#include "sloked/core/Hash.h"

namespace sloked {

    template <typename T>
    constexpr uint8_t ByteAt(T value, std::size_t idx) {
        assert(idx < sizeof(T));
        return (value >> (idx << 3)) & 0xff;
    }

    class SlokedCryptoSocket::Frame {
     public:
        enum class Type : uint8_t { Data = 0, KeyChange };

        Frame(Type type, SlokedSpan<const uint8_t> payload)
            : type(type), checksum(SlokedCrc32::Calculate(
                              payload.Data(), payload.Data() + payload.Size())),
              payload(payload.Data(), payload.Data() + payload.Size()) {}

        Type GetType() const {
            return this->type;
        }

        SlokedCrc32::Checksum GetChecksum() const {
            return this->checksum;
        }

        const std::vector<uint8_t> &GetPayload() const {
            return this->payload;
        }

        std::vector<uint8_t> Encrypt(SlokedCrypto::Cipher &cipher,
                                     SlokedCrypto::Key &key,
                                     SlokedCrypto::Random &random) const {
            if (!this->payload.empty()) {
                std::size_t totalLength = this->payload.size();
                if (this->payload.size() % cipher.Parameters().BlockSize != 0) {
                    totalLength =
                        (this->payload.size() / cipher.Parameters().BlockSize +
                         1) *
                        cipher.Parameters().BlockSize;
                }
                auto raw = this->payload;
                raw.insert(raw.end(), totalLength - this->payload.size(), 0);
                SlokedCrypto::Data init_vector;
                for (std::size_t i = 0; i < cipher.Parameters().IVSize; i++) {
                    init_vector.push_back(random.NextByte());
                }
                std::vector<uint8_t> encrypted;
                if (!raw.empty()) {
                    encrypted = cipher.Encrypt(raw, key, init_vector);
                }
                std::vector<uint8_t> header{static_cast<uint8_t>(this->type),
                                            ByteAt(this->payload.size(), 0),
                                            ByteAt(this->payload.size(), 1),
                                            ByteAt(this->payload.size(), 2),
                                            ByteAt(this->payload.size(), 3),
                                            ByteAt(this->checksum, 0),
                                            ByteAt(this->checksum, 1),
                                            ByteAt(this->checksum, 2),
                                            ByteAt(this->checksum, 3)};
                header.insert(header.end(), init_vector.begin(),
                              init_vector.end());
                encrypted.insert(encrypted.begin(), header.begin(),
                                 header.end());
                return encrypted;
            } else {
                std::vector<uint8_t> message{static_cast<uint8_t>(this->type),
                                             0u, 0u, 0u, 0u};
                return message;
            }
        }

        static std::optional<Frame> Decrypt(std::vector<uint8_t> &input,
                                            SlokedCrypto::Cipher &cipher,
                                            SlokedCrypto::Key &key) {
            constexpr std::size_t MinimalHeaderLength = 5;
            if (input.size() < MinimalHeaderLength) {
                return std::optional<Frame>{};
            }
            if (input.at(0) > static_cast<uint8_t>(Type::KeyChange)) {
                throw SlokedError("CryptoSocket: Invalid frame type");
            }
            Type type = static_cast<Type>(input.at(0));
            const std::size_t length = input.at(1) + (input.at(2) << 8) +
                                       (input.at(3) << 16) +
                                       (input.at(4) << 24);
            if (length > 0) {
                const std::size_t EncryptedHeaderSize =
                    9 + cipher.Parameters().IVSize;
                const SlokedCrc32::Checksum crc32 =
                    input.at(5) + (input.at(6) << 8) + (input.at(7) << 16) +
                    (input.at(8) << 24);
                std::size_t totalLength = length;
                if (length % cipher.Parameters().BlockSize != 0) {
                    totalLength = (length / cipher.Parameters().BlockSize + 1) *
                                  cipher.Parameters().BlockSize;
                }
                if (input.size() < totalLength + EncryptedHeaderSize) {
                    return std::optional<Frame>{};
                }

                SlokedCrypto::Data init_vector;
                init_vector.insert(init_vector.end(),
                                   input.begin() + EncryptedHeaderSize -
                                       cipher.Parameters().IVSize,
                                   input.begin() + EncryptedHeaderSize);
                std::vector<uint8_t> encrypted(
                    input.begin() + EncryptedHeaderSize,
                    input.begin() + EncryptedHeaderSize + totalLength);
                input.erase(input.begin(),
                            input.begin() + EncryptedHeaderSize + totalLength);
                const auto raw = cipher.Decrypt(encrypted, key, init_vector);
                auto actualCrc32 =
                    SlokedCrc32::Calculate(raw.begin(), raw.begin() + length);
                if (actualCrc32 != crc32) {
                    throw SlokedError(
                        "CryptoSocket: Actual CRC32 doesn't equal to expected");
                }
                return Frame(type, SlokedSpan(raw.data(), length));
            } else {
                input.erase(input.begin(), input.begin() + MinimalHeaderLength);
                return Frame(type, SlokedSpan<const uint8_t>(nullptr, 0));
            }
        }

     private:
        Frame(Type type, SlokedCrc32::Checksum checksum,
              std::vector<uint8_t> payload)
            : type(type), checksum(checksum), payload(payload) {}
        Type type;
        SlokedCrc32::Checksum checksum;
        std::vector<uint8_t> payload;
    };

    SlokedCryptoSocket::SlokedCryptoSocket(
        std::unique_ptr<SlokedSocket> socket,
        std::unique_ptr<SlokedCrypto::Cipher> cipher,
        std::unique_ptr<SlokedCrypto::Key> key,
        std::unique_ptr<SlokedCrypto::Random> random)
        : socket(std::move(socket)),
          cipher(std::move(cipher)), defaultKey{nullptr}, key(std::move(key)),
          random(std::move(random)) {}

    SlokedCryptoSocket::SlokedCryptoSocket(SlokedCryptoSocket &&socket)
        : socket(std::move(socket.socket)), cipher(std::move(socket.cipher)),
          defaultKey(std::move(socket.defaultKey)), key(std::move(socket.key)),
          encryptedBuffer(std::move(socket.encryptedBuffer)),
          buffer(std::move(socket.buffer)) {}

    SlokedCryptoSocket &SlokedCryptoSocket::operator=(
        SlokedCryptoSocket &&socket) {
        this->socket = std::move(socket.socket);
        this->cipher = std::move(socket.cipher);
        this->defaultKey = std::move(socket.defaultKey);
        this->key = std::move(socket.key);
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

    bool SlokedCryptoSocket::Closed() {
        if (this->Valid()) {
            this->Fetch();
            return this->socket->Closed();
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
            std::vector<uint8_t> msg(this->buffer.begin(),
                                     this->buffer.begin() + count);
            this->buffer.erase(this->buffer.begin(),
                               this->buffer.begin() + count);
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

    void SlokedCryptoSocket::Flush() {
        if (this->Valid()) {
            this->socket->Flush();
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

    void SlokedCryptoSocket::SetEncryption(
        std::unique_ptr<SlokedCrypto::Key> key,
        std::optional<std::string> keyId) {

        this->SendKeyChangeNofitication(keyId);
        if (this->defaultKey == nullptr) {
            this->defaultKey = std::move(this->key);
        }
        this->key = std::move(key);
    }

    void SlokedCryptoSocket::RestoreDefaultEncryption(
        std::optional<std::string> keyId) {
        this->SendKeyChangeNofitication(keyId);
        if (this->defaultKey) {
            this->key = std::move(this->defaultKey);
        }
    }

    std::function<void()> SlokedCryptoSocket::OnKeyChange(
        std::function<void(const std::optional<std::string> &)> listener) {
        return this->keyChangeEmitter.Listen(std::move(listener));
    }

    void SlokedCryptoSocket::SendKeyChangeNofitication(
        const std::optional<std::string> &keyId) {
        if (keyId.has_value()) {
            EncodingConverter conv(Encoding::Get("system"), Encoding::Utf8);
            auto encodedKeyId = conv.Convert(keyId.value());
            Frame frame(
                Frame::Type::KeyChange,
                SlokedSpan<const uint8_t>(
                    reinterpret_cast<const uint8_t *>(encodedKeyId.c_str()),
                    keyId->size()));
            auto encrypted =
                frame.Encrypt(*this->cipher, *this->key, *this->random);
            this->socket->Write(SlokedSpan(encrypted.data(), encrypted.size()));
            this->Flush();
        }
    }

    void SlokedCryptoSocket::Put(const uint8_t *bytes, std::size_t length) {
        Frame frame(Frame::Type::Data, SlokedSpan(bytes, length));
        auto encrypted =
            frame.Encrypt(*this->cipher, *this->key, *this->random);
        this->socket->Write(SlokedSpan(encrypted.data(), encrypted.size()));
    }

    void SlokedCryptoSocket::Fetch(std::size_t sz) {
        do {
            auto chunk = this->socket->Read(this->socket->Available());
            this->encryptedBuffer.insert(this->encryptedBuffer.end(),
                                         chunk.begin(), chunk.end());
            auto frame = Frame::Decrypt(this->encryptedBuffer, *this->cipher,
                                        *this->key);
            if (frame.has_value()) {
                this->InsertFrame(frame.value());
            }
        } while (this->buffer.size() < sz || this->socket->Available() > 0);
    }

    void SlokedCryptoSocket::InsertFrame(const Frame &frame) {
        switch (frame.GetType()) {
            case Frame::Type::Data:
                if (!frame.GetPayload().empty()) {
                    this->buffer.insert(this->buffer.end(),
                                        frame.GetPayload().begin(),
                                        frame.GetPayload().end());
                }
                break;

            case Frame::Type::KeyChange: {
                std::optional<std::string> param;
                if (!frame.GetPayload().empty()) {
                    EncodingConverter conv(Encoding::Utf8,
                                           Encoding::Get("system"));
                    param = conv.Convert(std::string(frame.GetPayload().begin(),
                                                     frame.GetPayload().end()));
                }
                this->keyChangeEmitter.Emit(param);
            } break;
        }
    }

    SlokedCryptoServerSocket::SlokedCryptoServerSocket(
        std::unique_ptr<SlokedServerSocket> serverSocket, SlokedCrypto &crypto,
        std::unique_ptr<SlokedCrypto::Key> key)
        : serverSocket(std::move(serverSocket)), crypto(crypto),
          defaultKey(std::move(key)) {}

    SlokedCryptoServerSocket::SlokedCryptoServerSocket(
        SlokedCryptoServerSocket &&serverSocket)
        : serverSocket(std::move(serverSocket.serverSocket)),
          crypto(serverSocket.crypto),
          defaultKey(std::move(serverSocket.defaultKey)) {}

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

    std::unique_ptr<SlokedSocket> SlokedCryptoServerSocket::Accept(
        std::chrono::system_clock::duration timeout) {
        if (this->Valid()) {
            auto rawSocket = this->serverSocket->Accept(timeout);
            if (rawSocket) {
                return std::make_unique<SlokedCryptoSocket>(
                    std::move(rawSocket), this->crypto.NewCipher(),
                    this->defaultKey->Clone(), this->crypto.NewRandom());
            } else {
                return nullptr;
            }
        } else {
            throw SlokedError("CryptoServerSocket: Invalid socket");
        }
    }

    std::unique_ptr<SlokedIOAwaitable> SlokedCryptoServerSocket::Awaitable()
        const {
        return this->serverSocket->Awaitable();
    }

    SlokedCryptoSocketFactory::SlokedCryptoSocketFactory(
        SlokedSocketFactory &socketFactory, SlokedCrypto &crypto,
        std::unique_ptr<SlokedCrypto::Key> key)
        : socketFactory(socketFactory), crypto(crypto),
          defaultKey(std::move(key)) {}

    SlokedCryptoSocketFactory::SlokedCryptoSocketFactory(
        SlokedCryptoSocketFactory &&socketFactory)
        : socketFactory(socketFactory.socketFactory),
          crypto(socketFactory.crypto),
          defaultKey(std::move(socketFactory.defaultKey)) {}

    std::unique_ptr<SlokedSocket> SlokedCryptoSocketFactory::Connect(
        const SlokedSocketAddress &addr) {
        auto rawSocket = this->socketFactory.Connect(addr);
        if (rawSocket) {
            return std::make_unique<SlokedCryptoSocket>(
                std::move(rawSocket), this->crypto.NewCipher(),
                this->defaultKey->Clone(), this->crypto.NewRandom());
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedServerSocket> SlokedCryptoSocketFactory::Bind(
        const SlokedSocketAddress &addr) {
        auto rawSocket = this->socketFactory.Bind(addr);
        if (rawSocket) {
            return std::make_unique<SlokedCryptoServerSocket>(
                std::move(rawSocket), this->crypto, this->defaultKey->Clone());
        } else {
            return nullptr;
        }
    }
}  // namespace sloked