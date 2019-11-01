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

#include "sloked/net/PosixSocket.h"
#include "sloked/core/Error.h"
#include "sloked/core/Scope.h"
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>

namespace sloked {

    constexpr bool IsSocketValid(int fd) {
        return fd >= 0;
    }

    SlokedPosixSocket::SlokedPosixSocket(int fd)
        : socket(fd) {
        
        int one = 1;
        setsockopt(fd, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
    }
        
    SlokedPosixSocket::SlokedPosixSocket(SlokedPosixSocket &&socket) {
        this->socket = socket.socket;
        socket.socket = InvalidSocket;
    }

    SlokedPosixSocket::~SlokedPosixSocket() {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
        }
    }

    SlokedPosixSocket &SlokedPosixSocket::operator=(SlokedPosixSocket &&socket) {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
        }
        this->socket = socket.socket;
        socket.socket = InvalidSocket;
        return *this;
    }

    void SlokedPosixSocket::Open(int fd) {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
        }
        this->socket = fd;
    }

    bool SlokedPosixSocket::Valid() {
        return IsSocketValid(this->socket);
    }

    void SlokedPosixSocket::Close() {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
            this->socket = InvalidSocket;
        }
    }

    std::size_t SlokedPosixSocket::Available() {
        if (IsSocketValid(this->socket)) {
            int size;
            ioctl(this->socket, FIONREAD, &size);
            return static_cast<std::size_t>(size);
        } else {
            throw SlokedError("PosixSocket: Invalid socket");
        }
    }

    bool SlokedPosixSocket::Wait(long timeout) {
        struct timeval tv;
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(this->socket, &rfds);
        
        tv.tv_sec = 0;
        tv.tv_usec = timeout * 1000;
        return select(this->socket + 1, &rfds, nullptr, nullptr, &tv) > 0;
    }

    std::optional<uint8_t> SlokedPosixSocket::Read() {
        if (IsSocketValid(this->socket)) {
            uint8_t byte;
            auto res = read(this->socket, &byte, 1);
            if (res > 0) {
                return byte;
            } else if (res == 0) {
                return {};
            } else {
                throw SlokedError("PosixSocket: Read error");
            }

        } else {
            throw SlokedError("PosixSocket: Invalid socket");
        }
    }

    std::vector<uint8_t> SlokedPosixSocket::Read(std::size_t count) {
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

    void SlokedPosixSocket::Write(SlokedSpan<const uint8_t> data) {
        if (IsSocketValid(this->socket)) {
            if (!data.Empty()) {
                auto res = write(this->socket, data.Data(), data.Size());
                if (res == -1) {
                    throw SlokedError("PosixSocket: Write error");
                }
            }
        } else {
            throw SlokedError("PosixSocket: Invalid socket");
        }
    }

    void SlokedPosixSocket::Write(uint8_t byte) {
        this->Write(SlokedSpan<const uint8_t>(&byte, 1));
    }

    SlokedPosixServerSocket::SlokedPosixServerSocket(int fd)
        : socket(fd) {}

    SlokedPosixServerSocket::SlokedPosixServerSocket(SlokedPosixServerSocket &&socket) {
        this->socket = socket.socket;
        socket.socket = InvalidSocket; 
    }

    SlokedPosixServerSocket::~SlokedPosixServerSocket() {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
        }
    }

    SlokedPosixServerSocket &SlokedPosixServerSocket::operator=(SlokedPosixServerSocket &&socket) {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
        }
        this->socket = socket.socket;
        socket.socket = InvalidSocket;
        return *this;
    }

    void SlokedPosixServerSocket::Open(int fd) {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
        }
        this->socket = fd;
    }

    bool SlokedPosixServerSocket::Valid() {
        return IsSocketValid(this->socket);
    }

    void SlokedPosixServerSocket::Start() {
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

    void SlokedPosixServerSocket::Close() {
        if (IsSocketValid(this->socket)) {
            close(this->socket);
            this->socket = InvalidSocket;
        }
    }

    std::unique_ptr<SlokedSocket> SlokedPosixServerSocket::Accept(long timeout) {
        int socket = InvalidSocket;
        if (timeout != 0) {
            struct timeval tv;
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(this->socket, &rfds);
            
            tv.tv_sec = 0;
            tv.tv_usec = timeout * 1000;
            auto result = select(this->socket + 1, &rfds, nullptr, nullptr, &tv);
            if (result > 0) {
                socket = accept(this->socket, nullptr, nullptr);
            }
        } else {
            socket = accept(this->socket, nullptr, nullptr);
        }
        if (IsSocketValid(socket)) {
            return std::make_unique<SlokedPosixSocket>(socket);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<SlokedSocket> SlokedPosixSocketFactory::Connect(const std::string &host, uint16_t port) {
        // Resolve hostname
        struct addrinfo *result = nullptr;
        int err = getaddrinfo(host.c_str(), nullptr, nullptr, &result);
        if (err != 0 || result == nullptr) {
            throw SlokedError("PosixSocket: Error connecting to " + host + ":" + std::to_string(port) + "; address resolution error: " + std::to_string(err));
        }
        OnDestroy freeResult([&result] {
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
                        return std::make_unique<SlokedPosixSocket>(fd);
                    }
                } break;

                case AF_INET6: {
                    sockaddr_in6 addr = *reinterpret_cast<sockaddr_in6 *>(result->ai_addr);
                    addr.sin6_port = htons(port);
                    err = connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
                    if (err == 0) {
                        return std::make_unique<SlokedPosixSocket>(fd);
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

    std::unique_ptr<SlokedServerSocket> SlokedPosixSocketFactory::Bind(const std::string &host, uint16_t port) {
        // Resolve hostname
        struct addrinfo *result = nullptr;
        int err = getaddrinfo(host.c_str(), nullptr, nullptr, &result);
        if (err != 0 || result == nullptr) {
            throw SlokedError("PosixSocket: Error connecting to " + host + ":" + std::to_string(port) + "; address resolution error: " + std::to_string(err));
        }
        OnDestroy freeResult([&result] {
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
        return std::make_unique<SlokedPosixServerSocket>(fd);
    }
}