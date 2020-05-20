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

#include "sloked/net/Win32Socket.h"

#include <Winsock2.h>
#include <WS2tcpip.h>

#include "sloked/core/Error.h"
#include "sloked/core/Scope.h"
#include "sloked/core/win32/Time.h"

namespace sloked {

    constexpr bool IsSocketValid(int fd) {
        return fd >= 0;
    }

    SlokedWin32Socket::SlokedWin32Socket(int fd) : socket(fd) {

        const char one = 1;
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) != 0) {
            throw SlokedError("Win32Socket: Error setting socket options");
        }
    }

    SlokedWin32Socket::SlokedWin32Socket(SlokedWin32Socket &&socket) {
        this->socket = socket.socket;
        socket.socket = InvalidSocket;
    }

    SlokedWin32Socket::~SlokedWin32Socket() {
        if (IsSocketValid(this->socket)) {
            closesocket(this->socket);
        }
    }

    SlokedWin32Socket &SlokedWin32Socket::operator=(
        SlokedWin32Socket &&socket) {
        if (IsSocketValid(this->socket)) {
            closesocket(this->socket);
        }
        this->socket = socket.socket;
        socket.socket = InvalidSocket;
        return *this;
    }

    void SlokedWin32Socket::Open(int fd) {
        if (IsSocketValid(this->socket)) {
            closesocket(this->socket);
        }
        this->socket = fd;
    }

    bool SlokedWin32Socket::Valid() {
        return IsSocketValid(this->socket);
    }

    void SlokedWin32Socket::Close() {
        if (IsSocketValid(this->socket)) {
            closesocket(this->socket);
            this->socket = InvalidSocket;
        }
    }

    std::size_t SlokedWin32Socket::Available() {
        if (IsSocketValid(this->socket)) {
            unsigned long size;
            ioctlsocket(this->socket, FIONREAD, &size);
            return static_cast<std::size_t>(size);
        } else {
            throw SlokedError("Win32Socket: Invalid socket");
        }
    }

    bool SlokedWin32Socket::Closed() {
        if (IsSocketValid(this->socket)) {
            unsigned long size;
            ioctlsocket(this->socket, FIONREAD, &size);
            if (size == 0) {
                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 0;
                fd_set rfds;
                FD_ZERO(&rfds);
                FD_SET(this->socket, &rfds);
                return select(this->socket + 1, &rfds, nullptr, nullptr, &tv) >
                       0;
            } else {
                return false;
            }
        } else {
            throw SlokedError("Win32Socket: Invalid socket");
        }
    }

    bool SlokedWin32Socket::Wait(std::chrono::system_clock::duration timeout) {
        if (IsSocketValid(this->socket)) {
            struct timeval tv;
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(this->socket, &rfds);

            DurationToTimeval(timeout, tv);
            return select(this->socket + 1, &rfds, nullptr, nullptr, &tv) > 0;
        } else {
            return false;
        }
    }

    std::optional<uint8_t> SlokedWin32Socket::Read() {
        if (IsSocketValid(this->socket)) {
            char byte;
            auto res = recv(this->socket, &byte, 1, 0);
            if (res > 0) {
                return static_cast<uint8_t>(byte);
            } else if (res == 0) {
                return {};
            } else {
                this->Close();
                throw SlokedError("Win32Socket: Read error");
            }
        } else {
            throw SlokedError("Win32Socket: Invalid socket");
        }
    }

    std::vector<uint8_t> SlokedWin32Socket::Read(std::size_t count) {
        if (IsSocketValid(this->socket)) {
            std::vector<uint8_t> dest;
            std::unique_ptr<char[]> buffer(new char[count]);
            auto res = recv(this->socket, buffer.get(), count, 0);
            if (res != -1) {
                dest.insert(dest.end(), buffer.get(), buffer.get() + res);
                return dest;
            } else {
                this->Close();
                throw SlokedError("Win32Socket: Read error");
            }

        } else {
            throw SlokedError("Win32Socket: Invalid socket");
        }
    }

    void SlokedWin32Socket::Write(SlokedSpan<const uint8_t> data) {
        if (IsSocketValid(this->socket)) {
            if (!data.Empty()) {
                auto res = send(this->socket, reinterpret_cast<const char *>(data.Data()), data.Size(), 0);
                if (res == -1) {
                    this->Close();
                    throw SlokedError("Win32Socket: Write error");
                }
            }
        } else {
            throw SlokedError("Win32Socket: Invalid socket");
        }
    }

    void SlokedWin32Socket::Write(uint8_t byte) {
        this->Write(SlokedSpan<const uint8_t>(&byte, 1));
    }

    void SlokedWin32Socket::Flush() {
        if (IsSocketValid(this->socket)) {
            // TODO
        } else {
            throw SlokedError("Win32Socket: Invalid socket");
        }
    }

    std::unique_ptr<SlokedIOAwaitable> SlokedWin32Socket::Awaitable() const {
        return std::make_unique<SlokedWin32Awaitable>(this->socket);
    }

    SlokedWin32ServerSocket::SlokedWin32ServerSocket(int fd) : socket(fd) {}

    SlokedWin32ServerSocket::SlokedWin32ServerSocket(
        SlokedWin32ServerSocket &&socket) {
        this->socket = socket.socket;
        socket.socket = InvalidSocket;
    }

    SlokedWin32ServerSocket::~SlokedWin32ServerSocket() {
        if (IsSocketValid(this->socket)) {
            closesocket(this->socket);
        }
    }

    SlokedWin32ServerSocket &SlokedWin32ServerSocket::operator=(
        SlokedWin32ServerSocket &&socket) {
        if (IsSocketValid(this->socket)) {
            closesocket(this->socket);
        }
        this->socket = socket.socket;
        socket.socket = InvalidSocket;
        return *this;
    }

    void SlokedWin32ServerSocket::Open(int fd) {
        if (IsSocketValid(this->socket)) {
            closesocket(this->socket);
        }
        this->socket = fd;
    }

    bool SlokedWin32ServerSocket::Valid() {
        return IsSocketValid(this->socket);
    }

    void SlokedWin32ServerSocket::Start() {
        constexpr int MaxQueueLength = 128;
        if (IsSocketValid(this->socket)) {
            auto res = listen(this->socket, MaxQueueLength);
            if (res != 0) {
                this->Close();
                throw SlokedError(
                    "Win32ServerSocket: Error starting server \'" +
                    std::to_string(errno) + "\'");
            }
        } else {
            throw SlokedError("Win32ServerSocket: Invalid socket");
        }
    }

    void SlokedWin32ServerSocket::Close() {
        if (IsSocketValid(this->socket)) {
            closesocket(this->socket);
            this->socket = InvalidSocket;
        }
    }

    std::unique_ptr<SlokedSocket> SlokedWin32ServerSocket::Accept(
        std::chrono::system_clock::duration timeout) {
        int socket = InvalidSocket;
        if (timeout != std::chrono::system_clock::duration::zero()) {
            struct timeval tv;
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(this->socket, &rfds);

            DurationToTimeval(timeout, tv);
            auto result =
                select(this->socket + 1, &rfds, nullptr, nullptr, &tv);
            if (result > 0) {
                socket = accept(this->socket, nullptr, nullptr);
            }
        } else {
            socket = accept(this->socket, nullptr, nullptr);
        }
        if (IsSocketValid(socket)) {
            return std::make_unique<SlokedWin32Socket>(socket);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedIOAwaitable> SlokedWin32ServerSocket::Awaitable()
        const {
        return std::make_unique<SlokedWin32Awaitable>(this->socket);
    }

    std::unique_ptr<SlokedSocket> SlokedWin32SocketFactory::Connect(
        const SlokedSocketAddress &addr) {
        const auto &host = addr.AsNetwork().host;
        const auto &port = addr.AsNetwork().port;
        // Resolve hostname
        struct addrinfo *result = nullptr;
        struct addrinfo hints {
            0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, nullptr, nullptr, nullptr
        };
        int err = getaddrinfo(host.c_str(), nullptr, &hints, &result);
        if (err != 0 || result == nullptr) {
            throw SlokedError(
                "Win32Socket: Error connecting to " + host + ":" +
                std::to_string(port) +
                "; address resolution error: " + std::to_string(err));
        }
        AtScopeExit freeResult([&result] { freeaddrinfo(result); });
        // Open a socket
        int fd =
            socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (!IsSocketValid(fd)) {
            throw SlokedError(
                "Win32Socket: Error connecting to " + host + ":" +
                std::to_string(port) +
                "; socket opening error: " + std::to_string(errno));
        }
        // Connect to the socket through one of resolutions
        for (auto res = result; res != nullptr; res = res->ai_next) {
            switch (result->ai_family) {
                case AF_INET: {
                    sockaddr_in addr =
                        *reinterpret_cast<sockaddr_in *>(result->ai_addr);
                    addr.sin_port = htons(port);
                    err = connect(fd, reinterpret_cast<sockaddr *>(&addr),
                                  sizeof(addr));
                    if (err == 0) {
                        return std::make_unique<SlokedWin32Socket>(fd);
                    }
                } break;

                case AF_INET6: {
                    sockaddr_in6 addr =
                        *reinterpret_cast<sockaddr_in6 *>(result->ai_addr);
                    addr.sin6_port = htons(port);
                    err = connect(fd, reinterpret_cast<sockaddr *>(&addr),
                                  sizeof(addr));
                    if (err == 0) {
                        return std::make_unique<SlokedWin32Socket>(fd);
                    }
                } break;

                default:
                    break;
            }
            if (err != 0) {
                closesocket(fd);
                throw SlokedError(
                    "Win32Socket: Error connecting to " + host + ":" +
                    std::to_string(port) +
                    "; connection error: " + std::to_string(errno));
            }
        }
        closesocket(fd);
        throw SlokedError("Win32Socket: Error connecting to " + host + ":" +
                          std::to_string(port) + "; unsupported AF");
    }

    std::unique_ptr<SlokedServerSocket> SlokedWin32SocketFactory::Bind(
        const SlokedSocketAddress &addr) {
        const auto &host = addr.AsNetwork().host;
        const auto &port = addr.AsNetwork().port;
        // Resolve hostname
        struct addrinfo *result = nullptr;
        struct addrinfo hints {
            0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, nullptr, nullptr, nullptr
        };
        int err = getaddrinfo(host.c_str(), nullptr, &hints, &result);
        if (err != 0 || result == nullptr) {
            throw SlokedError(
                "Win32Socket: Error connecting to " + host + ":" +
                std::to_string(port) +
                "; address resolution error: " + std::to_string(err));
        }
        AtScopeExit freeResult([&result] { freeaddrinfo(result); });
        // Open a socket
        int fd =
            socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (!IsSocketValid(fd)) {
            throw SlokedError(
                "Win32Socket: Error connecting to " + host + ":" +
                std::to_string(port) +
                "; socket opening error: " + std::to_string(errno));
        }
        const char Enable = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &Enable, sizeof(Enable)) != 0) {
            closesocket(fd);
            throw SlokedError("Win32Socket: Error setting socket options");
        }
        // Bind to the socket
        switch (result->ai_family) {
            case AF_INET: {
                sockaddr_in addr =
                    *reinterpret_cast<sockaddr_in *>(result->ai_addr);
                addr.sin_port = htons(port);
                err =
                    bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
            } break;

            case AF_INET6: {
                sockaddr_in6 addr =
                    *reinterpret_cast<sockaddr_in6 *>(result->ai_addr);
                addr.sin6_port = htons(port);
                err =
                    bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
            } break;

            default:
                closesocket(fd);
                throw SlokedError(
                    "Win32Socket: Error connecting to " + host + ":" +
                    std::to_string(port) +
                    "; unsupported AF: " + std::to_string(result->ai_family));
        }
        if (err != 0) {
            closesocket(fd);
            throw SlokedError("Win32Socket: Error connecting to " + host + ":" +
                              std::to_string(port) +
                              "; binding error: " + std::to_string(errno));
        }
        // Build an instance of the server
        return std::make_unique<SlokedWin32ServerSocket>(fd);
    }
}  // namespace sloked
