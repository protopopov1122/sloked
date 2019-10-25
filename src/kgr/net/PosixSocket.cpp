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

#include "sloked/kgr/net/PosixSocket.h"
#include "sloked/core/Error.h"
#include "sloked/core/Scope.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

namespace sloked {

    constexpr bool IsSocketValid(int fd) {
        return fd >= 0;
    }

    KgrPosixSocket::KgrPosixSocket(int fd)
        : socket(fd) {}
        
    KgrPosixSocket::KgrPosixSocket(KgrPosixSocket &&socket) {
        this->socket = socket.socket;
        socket.socket = InvalidSocket;
    }

    KgrPosixSocket::~KgrPosixSocket() {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
        }
    }

    KgrPosixSocket &KgrPosixSocket::operator=(KgrPosixSocket &&socket) {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
        }
        this->socket = socket.socket;
        socket.socket = InvalidSocket;
        return *this;
    }

    void KgrPosixSocket::Open(int fd) {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
        }
        this->socket = fd;
    }

    bool KgrPosixSocket::IsValid() {
        return IsSocketValid(this->socket);
    }

    void KgrPosixSocket::Close() {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
            this->socket = InvalidSocket;
        }
    }

    std::vector<uint8_t> KgrPosixSocket::Read(std::size_t count) {
        if (IsSocketValid(this->socket)) {
            std::vector<uint8_t> dest;
            std::unique_ptr<uint8_t[]> buffer(new uint8_t[count]);
            auto res = read(this->socket, buffer.get(), count);
            if (res != -1) {
                dest.insert(dest.end(), buffer.get(), buffer.get() + res);
                return dest;
            } else {
                throw SlokedError("PosixSocket: Read error");
            }

        } else {
            throw SlokedError("PosixSocket: Invalid socket");
        }
    }

    void KgrPosixSocket::Write(const uint8_t *src, std::size_t count) {
        if (IsSocketValid(this->socket)) {
            auto res = write(this->socket, src, count);
            if (res == -1) {
                throw SlokedError("PosixSocket: Write error");
            }

        } else {
            throw SlokedError("PosixSocket: Invalid socket");
        }
    }

    KgrPosixServerSocket::KgrPosixServerSocket(int fd)
        : socket(fd) {}

    KgrPosixServerSocket::KgrPosixServerSocket(KgrPosixServerSocket &&socket) {
        this->socket = socket.socket;
        socket.socket = InvalidSocket; 
    }

    KgrPosixServerSocket::~KgrPosixServerSocket() {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
        }
    }

    KgrPosixServerSocket &KgrPosixServerSocket::operator=(KgrPosixServerSocket &&socket) {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
        }
        this->socket = socket.socket;
        socket.socket = InvalidSocket;
        return *this;
    }

    void KgrPosixServerSocket::Open(int fd) {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
        }
        this->socket = fd;
    }

    bool KgrPosixServerSocket::IsValid() {
        return IsSocketValid(this->socket);
    }

    void KgrPosixServerSocket::Start() {
        constexpr int MaxQueueLength = 128;
        if (IsSocketValid(this->socket)) {
            auto res = listen(this->socket, MaxQueueLength);
            if (res != 0) {
                throw SlokedError("PosixServerSocket: Error starting server");
            }
        } else {
            throw SlokedError("PosixServerSocket: Invalid socket");
        }
    }

    void KgrPosixServerSocket::Close() {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
            this->socket = InvalidSocket;
        }
    }

    std::unique_ptr<KgrSocket> KgrPosixServerSocket::Accept() {
        int socket = accept(this->socket, nullptr, nullptr);
        if (IsSocketValid(socket)) {
            return std::make_unique<KgrPosixSocket>(socket);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<KgrSocket> KgrPosixSocketFactory::Connect(const std::string &host, uint16_t port) {
        // Resolve hostname
        struct addrinfo *result = nullptr;
        int err = getaddrinfo(host.c_str(), nullptr, nullptr, &result);
        if (err != 0 || result == nullptr) {
            throw SlokedError("PosixSocket: Error connecting to " + host + ":" + std::to_string(port) + "; address resolution error: " + std::to_string(err));
        }
        OnScopeExit freeResult([&result] {
            freeaddrinfo(result);
        });
        // Open a socket
        int fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (!IsSocketValid(fd)) {
            throw SlokedError("PosixSocket: Error connecting to " + host + ":" + std::to_string(port) + "; socket opening error: " + std::to_string(errno));
        }
        // Connect to the socket through one of resolutions
        for (auto res = result; res != nullptr; res = res->ai_next) {
            switch (result->ai_family) {
                case AF_INET: {
                    sockaddr_in addr = *reinterpret_cast<sockaddr_in *>(result->ai_addr);
                    addr.sin_port = htons(port);
                    err = connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
                    if (err == 0) {
                        return std::make_unique<KgrPosixSocket>(fd);
                    }
                } break;

                case AF_INET6: {
                    sockaddr_in6 addr = *reinterpret_cast<sockaddr_in6 *>(result->ai_addr);
                    addr.sin6_port = htons(port);
                    err = connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
                    if (err == 0) {
                        return std::make_unique<KgrPosixSocket>(fd);
                    }
                } break;

                default:
                    break;
            }
            if (err != 0) {
                throw SlokedError("PosixSocket: Error connecting to " + host + ":" + std::to_string(port) + "; connection error: " + std::to_string(errno));
            }
        }
        throw SlokedError("PosixSocket: Error connecting to " + host + ":" + std::to_string(port) + "; unsupported AF");
    }

    std::unique_ptr<KgrServerSocket> KgrPosixSocketFactory::Bind(const std::string &host, uint16_t port) {
        // Resolve hostname
        struct addrinfo *result = nullptr;
        int err = getaddrinfo(host.c_str(), nullptr, nullptr, &result);
        if (err != 0 || result == nullptr) {
            throw SlokedError("PosixSocket: Error connecting to " + host + ":" + std::to_string(port) + "; address resolution error: " + std::to_string(err));
        }
        OnScopeExit freeResult([&result] {
            freeaddrinfo(result);
        });
        // Open a socket
        int fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (!IsSocketValid(fd)) {
            throw SlokedError("PosixSocket: Error connecting to " + host + ":" + std::to_string(port) + "; socket opening error: " + std::to_string(errno));
        }
        const int Enable = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &Enable, sizeof(Enable));
        // Bind to the socket
        switch (result->ai_family) {
            case AF_INET: {
                sockaddr_in addr = *reinterpret_cast<sockaddr_in *>(result->ai_addr);
                addr.sin_port = htons(port);
                err = bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
            } break;

            case AF_INET6: {
                sockaddr_in6 addr = *reinterpret_cast<sockaddr_in6 *>(result->ai_addr);
                addr.sin6_port = htons(port);
                err = bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
            } break;

            default:
                throw SlokedError("PosixSocket: Error connecting to " + host + ":" + std::to_string(port) + "; unsupported AF: " + std::to_string(result->ai_family));
        }
        if (err != 0) {
            throw SlokedError("PosixSocket: Error connecting to " + host + ":" + std::to_string(port) + "; binding error: " + std::to_string(errno));
        }
        // Build an instance of the server
        return std::make_unique<KgrPosixServerSocket>(fd);
    }
}