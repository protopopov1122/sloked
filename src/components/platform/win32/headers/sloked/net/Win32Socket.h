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

#ifndef SLOKED_NET_WIN32SOCKET_H_
#define SLOKED_NET_WIN32SOCKET_H_

#include <map>
#include <mutex>

#include "sloked/core/awaitable/Win32.h"
#include "sloked/net/Socket.h"

namespace sloked {

    class SlokedWin32Socket : public SlokedSocket {
     public:
        SlokedWin32Socket(int = InvalidSocket);
        SlokedWin32Socket(const SlokedWin32Socket &) = delete;
        SlokedWin32Socket(SlokedWin32Socket &&);
        virtual ~SlokedWin32Socket();

        SlokedWin32Socket &operator=(const SlokedWin32Socket &) = delete;
        SlokedWin32Socket &operator=(SlokedWin32Socket &&);

        void Open(int);

        bool Valid() final;
        void Close() final;
        std::size_t Available() final;
        bool Closed() final;
        bool Wait(std::chrono::system_clock::duration =
                      std::chrono::system_clock::duration::zero()) final;
        std::optional<uint8_t> Read() final;
        std::vector<uint8_t> Read(std::size_t) final;
        void Write(SlokedSpan<const uint8_t>) final;
        void Write(uint8_t) final;
        void Flush() final;
        std::unique_ptr<SlokedIOAwaitable> Awaitable() const final;

        static constexpr int InvalidSocket = -1;

     private:
        int socket;
    };

    class SlokedWin32ServerSocket : public SlokedServerSocket {
     public:
        SlokedWin32ServerSocket(int = InvalidSocket);
        SlokedWin32ServerSocket(const SlokedWin32ServerSocket &) = delete;
        SlokedWin32ServerSocket(SlokedWin32ServerSocket &&);
        virtual ~SlokedWin32ServerSocket();

        SlokedWin32ServerSocket &operator=(const SlokedWin32ServerSocket &) =
            delete;
        SlokedWin32ServerSocket &operator=(SlokedWin32ServerSocket &&);

        void Open(int);

        bool Valid() final;
        void Start() final;
        void Close() final;
        std::unique_ptr<SlokedSocket> Accept(
            std::chrono::system_clock::duration =
                std::chrono::system_clock::duration::zero()) final;
        std::unique_ptr<SlokedIOAwaitable> Awaitable() const final;

        static constexpr int InvalidSocket = -1;

     private:
        int socket;
    };

    class SlokedWin32SocketFactory : public SlokedSocketFactory {
     public:
        SlokedWin32SocketFactory() = default;
        std::unique_ptr<SlokedSocket> Connect(
            const SlokedSocketAddress &) final;
        std::unique_ptr<SlokedServerSocket> Bind(
            const SlokedSocketAddress &) final;
    };
}  // namespace sloked

#endif