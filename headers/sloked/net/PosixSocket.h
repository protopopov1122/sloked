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

#ifndef SLOKED_NET_POSIXSOCKET_H_
#define SLOKED_NET_POSIXSOCKET_H_

#include "sloked/net/Socket.h"
#include "sloked/core/awaitable/Posix.h"
#include <map>
#include <mutex>

namespace sloked {

    class SlokedPosixSocket : public SlokedSocket {
     public:
        SlokedPosixSocket(int = InvalidSocket);
        SlokedPosixSocket(const SlokedPosixSocket &) = delete;
        SlokedPosixSocket(SlokedPosixSocket &&);
        virtual ~SlokedPosixSocket();

        SlokedPosixSocket &operator=(const SlokedPosixSocket &) = delete;
        SlokedPosixSocket &operator=(SlokedPosixSocket &&);

        void Open(int);

        bool Valid() final;
        void Close() final;
        std::size_t Available() final;
        bool Wait(long) final;
        std::optional<uint8_t> Read() final;
        std::vector<uint8_t> Read(std::size_t) final;
        void Write(SlokedSpan<const uint8_t>) final;
        void Write(uint8_t) final;
        std::unique_ptr<SlokedIOAwaitable> Awaitable() const final;
        
        static constexpr int InvalidSocket = -1;

     private:
        int socket;
    };

    class SlokedPosixServerSocket : public SlokedServerSocket {
     public:
        SlokedPosixServerSocket(int = InvalidSocket);
        SlokedPosixServerSocket(const SlokedPosixServerSocket &) = delete;
        SlokedPosixServerSocket(SlokedPosixServerSocket &&);
        virtual ~SlokedPosixServerSocket();

        SlokedPosixServerSocket &operator=(const SlokedPosixServerSocket &) = delete;
        SlokedPosixServerSocket &operator=(SlokedPosixServerSocket &&);

        void Open(int);

        bool Valid() final;
        void Start() final;
        void Close() final;
        std::unique_ptr<SlokedSocket> Accept(long = 0) final;
        std::unique_ptr<SlokedIOAwaitable> Awaitable() const final;

        static constexpr int InvalidSocket = -1;

     private:
        int socket;
    };

    class SlokedPosixSocketFactory : public SlokedSocketFactory {
     public:
        SlokedPosixSocketFactory() = default;
        std::unique_ptr<SlokedSocket> Connect(const std::string &, uint16_t) final;
        std::unique_ptr<SlokedServerSocket> Bind(const std::string &, uint16_t) final;
    };
}

#endif