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

#include "sloked/net/CompressedSocket.h"

#include <cassert>

#include "sloked/core/Error.h"

namespace sloked {

    SlokedCompressedSocket::SlokedCompressedSocket(
        std::unique_ptr<SlokedSocket> socket,
        std::unique_ptr<SlokedCompression::Compressor> compressor)
        : socket(std::move(socket)), compressor(std::move(compressor)) {}

    SlokedCompressedSocket::SlokedCompressedSocket(
        SlokedCompressedSocket &&socket)
        : socket(std::move(socket.socket)),
          compressor(std::move(socket.compressor)),
          compressedBuffer(std::move(socket.compressedBuffer)),
          buffer(std::move(socket.buffer)) {}

    SlokedCompressedSocket &SlokedCompressedSocket::operator=(
        SlokedCompressedSocket &&socket) {
        this->socket = std::move(socket.socket);
        this->compressor = std::move(socket.compressor);
        this->compressedBuffer = std::move(socket.compressedBuffer);
        this->buffer = std::move(socket.buffer);
        return *this;
    }

    bool SlokedCompressedSocket::Valid() {
        return this->socket != nullptr && this->socket->Valid();
    }

    void SlokedCompressedSocket::Close() {
        if (this->socket) {
            this->socket->Close();
            this->socket = nullptr;
            this->compressor = nullptr;
        }
    }

    std::size_t SlokedCompressedSocket::Available() {
        if (this->Valid()) {
            this->Fetch();
            return this->buffer.size();
        } else {
            throw SlokedError("CompressedSocket: Invalid socket");
        }
    }

    bool SlokedCompressedSocket::Closed() {
        if (this->Valid()) {
            this->Fetch();
            return this->socket->Closed();
        } else {
            throw SlokedError("CompressedSocket: Invalid socket");
        }
    }

    bool SlokedCompressedSocket::Wait(
        std::chrono::system_clock::duration timeout) {
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

    std::optional<uint8_t> SlokedCompressedSocket::Read() {
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
            throw SlokedError("CompressedSocket: Invalid socket");
        }
    }

    std::vector<uint8_t> SlokedCompressedSocket::Read(std::size_t count) {
        if (this->Valid()) {
            this->Fetch(count);
            std::vector<uint8_t> msg(this->buffer.begin(),
                                     this->buffer.begin() + count);
            this->buffer.erase(this->buffer.begin(),
                               this->buffer.begin() + count);
            return msg;
        } else {
            throw SlokedError("CompressedSocket: Invalid socket");
        }
    }

    void SlokedCompressedSocket::Write(SlokedSpan<const uint8_t> data) {
        if (this->Valid()) {
            this->Put(data.Data(), data.Size());
        } else {
            throw SlokedError("CompressedSocket: Invalid socket");
        }
    }

    void SlokedCompressedSocket::Write(uint8_t data) {
        if (this->Valid()) {
            this->Put(&data, 1);
        } else {
            throw SlokedError("CompressedSocket: Invalid socket");
        }
    }

    void SlokedCompressedSocket::Flush() {
        if (this->Valid()) {
            this->socket->Flush();
        } else {
            throw SlokedError("CompressedSocket: Invalid socket");
        }
    }

    std::unique_ptr<SlokedIOAwaitable> SlokedCompressedSocket::Awaitable()
        const {
        return this->socket->Awaitable();
    }

    SlokedSocketEncryption *SlokedCompressedSocket::GetEncryption() {
        if (this->Valid()) {
            return this->socket->GetEncryption();
        } else {
            throw SlokedError("CompressedSocket: Invalid socket");
        }
    }

    template <typename T>
    constexpr uint8_t ByteAt(T value, std::size_t idx) {
        assert(idx < sizeof(T));
        return (value >> (idx << 3)) & 0xff;
    }

    void SlokedCompressedSocket::Put(const uint8_t *bytes, std::size_t length) {
        auto compressed = this->compressor->Compress(SlokedSpan(bytes, length));
        std::vector<uint8_t> header{ByteAt(length, 0),
                                    ByteAt(length, 1),
                                    ByteAt(length, 2),
                                    ByteAt(length, 3),
                                    ByteAt(compressed.size(), 0),
                                    ByteAt(compressed.size(), 1),
                                    ByteAt(compressed.size(), 2),
                                    ByteAt(compressed.size(), 3)};
        compressed.insert(compressed.begin(), header.begin(), header.end());
        this->socket->Write(SlokedSpan(compressed.data(), compressed.size()));
    }

    void SlokedCompressedSocket::Fetch(std::size_t sz) {
        const std::size_t CompressedHeaderSize = 8;
        do {
            auto chunk = this->socket->Read(this->socket->Available());
            this->compressedBuffer.insert(this->compressedBuffer.end(),
                                          chunk.begin(), chunk.end());
            if (this->compressedBuffer.size() < CompressedHeaderSize) {
                continue;
            }

            std::size_t length = this->compressedBuffer.at(0) +
                                 (this->compressedBuffer.at(1) << 8) +
                                 (this->compressedBuffer.at(2) << 16) +
                                 (this->compressedBuffer.at(3) << 24);
            std::size_t compressedLength =
                this->compressedBuffer.at(4) +
                (this->compressedBuffer.at(5) << 8) +
                (this->compressedBuffer.at(6) << 16) +
                (this->compressedBuffer.at(7) << 24);
            if (this->compressedBuffer.size() <
                compressedLength + CompressedHeaderSize) {
                continue;
            }

            auto raw = this->compressor->Decompress(
                SlokedSpan(this->compressedBuffer.data() + CompressedHeaderSize,
                           compressedLength),
                length);
            this->compressedBuffer.erase(this->compressedBuffer.begin(),
                                         this->compressedBuffer.begin() +
                                             CompressedHeaderSize +
                                             compressedLength);
            this->buffer.insert(this->buffer.end(), raw.begin(),
                                raw.begin() + length);
        } while (this->buffer.size() < sz || this->socket->Available() > 0);
    }

    SlokedCompressedServerSocket::SlokedCompressedServerSocket(
        std::unique_ptr<SlokedServerSocket> serverSocket,
        SlokedCompression &compression)
        : serverSocket(std::move(serverSocket)), compression(compression) {}

    SlokedCompressedServerSocket::SlokedCompressedServerSocket(
        SlokedCompressedServerSocket &&serverSocket)
        : serverSocket(std::move(serverSocket.serverSocket)),
          compression(serverSocket.compression) {}

    bool SlokedCompressedServerSocket::Valid() {
        return this->serverSocket != nullptr && this->serverSocket->Valid();
    }

    void SlokedCompressedServerSocket::Start() {
        if (this->Valid()) {
            this->serverSocket->Start();
        } else {
            throw SlokedError("CompressedServerSocket: Invalid socket");
        }
    }

    void SlokedCompressedServerSocket::Close() {
        if (this->serverSocket) {
            this->serverSocket->Close();
            this->serverSocket = nullptr;
        }
    }

    std::unique_ptr<SlokedSocket> SlokedCompressedServerSocket::Accept(
        std::chrono::system_clock::duration timeout) {
        if (this->Valid()) {
            auto rawSocket = this->serverSocket->Accept(timeout);
            if (rawSocket) {
                return std::make_unique<SlokedCompressedSocket>(
                    std::move(rawSocket), this->compression.NewCompressor());
            } else {
                return nullptr;
            }
        } else {
            throw SlokedError("CompressedServerSocket: Invalid socket");
        }
    }

    std::unique_ptr<SlokedIOAwaitable> SlokedCompressedServerSocket::Awaitable()
        const {
        return this->serverSocket->Awaitable();
    }

    SlokedCompressedSocketFactory::SlokedCompressedSocketFactory(
        SlokedSocketFactory &socketFactory, SlokedCompression &compression)
        : socketFactory(socketFactory), compression(compression) {}

    SlokedCompressedSocketFactory::SlokedCompressedSocketFactory(
        SlokedCompressedSocketFactory &&socketFactory)
        : socketFactory(socketFactory.socketFactory),
          compression(socketFactory.compression) {}

    std::unique_ptr<SlokedSocket> SlokedCompressedSocketFactory::Connect(
        const SlokedSocketAddress &addr) {
        auto rawSocket = this->socketFactory.Connect(addr);
        if (rawSocket) {
            return std::make_unique<SlokedCompressedSocket>(
                std::move(rawSocket), this->compression.NewCompressor());
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedServerSocket> SlokedCompressedSocketFactory::Bind(
        const SlokedSocketAddress &addr) {
        auto rawSocket = this->socketFactory.Bind(addr);
        if (rawSocket) {
            return std::make_unique<SlokedCompressedServerSocket>(
                std::move(rawSocket), this->compression);
        } else {
            return nullptr;
        }
    }
}  // namespace sloked