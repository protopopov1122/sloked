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

#ifndef SLOKED_NET_COMPRESSEDSOCKET_H_
#define SLOKED_NET_COMPRESSEDSOCKET_H_

#include "sloked/core/Compression.h"
#include "sloked/net/Socket.h"

namespace sloked {

    class SlokedCompressedSocket : public SlokedSocket {
     public:
        SlokedCompressedSocket(std::unique_ptr<SlokedSocket>,
                               std::unique_ptr<SlokedCompression::Compressor>);
        SlokedCompressedSocket(const SlokedCompressedSocket &) = delete;
        SlokedCompressedSocket(SlokedCompressedSocket &&);
        SlokedCompressedSocket &operator=(const SlokedCompressedSocket &) =
            delete;
        SlokedCompressedSocket &operator=(SlokedCompressedSocket &&);

        bool Valid() final;
        void Close() final;
        std::size_t Available() final;
        bool Wait(std::chrono::system_clock::duration =
                      std::chrono::system_clock::duration::zero()) final;
        std::optional<uint8_t> Read() final;
        std::vector<uint8_t> Read(std::size_t) final;
        void Write(SlokedSpan<const uint8_t>) final;
        void Write(uint8_t) final;
        std::unique_ptr<SlokedIOAwaitable> Awaitable() const final;
        SlokedSocketEncryption *GetEncryption() final;

     private:
        void Put(const uint8_t *, std::size_t);
        void Fetch(std::size_t = 0);

        std::unique_ptr<SlokedSocket> socket;
        std::unique_ptr<SlokedCompression::Compressor> compressor;
        std::vector<uint8_t> compressedBuffer;
        std::vector<uint8_t> buffer;
    };

    class SlokedCompressedServerSocket : public SlokedServerSocket {
     public:
        SlokedCompressedServerSocket(std::unique_ptr<SlokedServerSocket>,
                                     SlokedCompression &);
        SlokedCompressedServerSocket(const SlokedCompressedServerSocket &) =
            delete;
        SlokedCompressedServerSocket(SlokedCompressedServerSocket &&);
        SlokedCompressedServerSocket &operator=(
            const SlokedCompressedServerSocket &) = delete;
        SlokedCompressedServerSocket &operator=(
            SlokedCompressedServerSocket &&) = delete;

        bool Valid() final;
        void Start() final;
        void Close() final;
        std::unique_ptr<SlokedSocket> Accept(
            std::chrono::system_clock::duration =
                std::chrono::system_clock::duration::zero()) final;
        std::unique_ptr<SlokedIOAwaitable> Awaitable() const final;

     private:
        std::unique_ptr<SlokedServerSocket> serverSocket;
        SlokedCompression &compression;
    };

    class SlokedCompressedSocketFactory : public SlokedSocketFactory {
     public:
        SlokedCompressedSocketFactory(SlokedSocketFactory &,
                                      SlokedCompression &);
        SlokedCompressedSocketFactory(const SlokedCompressedSocketFactory &) =
            delete;
        SlokedCompressedSocketFactory(SlokedCompressedSocketFactory &&);

        SlokedCompressedSocketFactory &operator=(
            const SlokedCompressedSocketFactory &) = delete;
        SlokedCompressedSocketFactory &operator=(
            SlokedCompressedSocketFactory &&) = delete;

        std::unique_ptr<SlokedSocket> Connect(
            const SlokedSocketAddress &) final;
        std::unique_ptr<SlokedServerSocket> Bind(
            const SlokedSocketAddress &) final;

     private:
        SlokedSocketFactory &socketFactory;
        SlokedCompression &compression;
    };
}  // namespace sloked

#endif