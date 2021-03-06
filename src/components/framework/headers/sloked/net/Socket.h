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

#ifndef SLOKED_NET_SOCKET_H_
#define SLOKED_NET_SOCKET_H_

#include <chrono>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "sloked/core/Crypto.h"
#include "sloked/core/Scope.h"
#include "sloked/core/Span.h"
#include "sloked/core/awaitable/Awaitable.h"

namespace sloked {

    class SlokedSocketEncryption {
     public:
        virtual ~SlokedSocketEncryption() = default;
        virtual std::unique_ptr<SlokedCrypto::Key> GetEncryptionKey() = 0;
        virtual void SetEncryptionKey(std::unique_ptr<SlokedCrypto::Key>,
                                      std::optional<std::string> = {}) = 0;
        virtual std::function<void()> OnKeyChange(
            std::function<void(const std::optional<std::string> &)>) = 0;
    };

    class SlokedSocket {
     public:
        SlokedSocket(const SlokedSocket &) = delete;
        SlokedSocket(SlokedSocket &&) = delete;
        virtual ~SlokedSocket() = default;
        SlokedSocket &operator=(const SlokedSocket &) = delete;
        SlokedSocket &operator=(SlokedSocket &&) = delete;

        virtual bool Valid() = 0;
        virtual void Close() = 0;
        virtual std::size_t Available() = 0;
        virtual bool Closed() = 0;
        virtual bool Wait(std::chrono::system_clock::duration =
                              std::chrono::system_clock::duration::zero()) = 0;
        virtual std::optional<uint8_t> Read() = 0;
        virtual std::vector<uint8_t> Read(std::size_t) = 0;
        virtual void Write(SlokedSpan<const uint8_t>) = 0;
        virtual void Write(uint8_t) = 0;
        virtual void Flush() = 0;
        virtual std::unique_ptr<SlokedIOAwaitable> Awaitable() const = 0;
        virtual SlokedSocketEncryption *GetEncryption();

     protected:
        SlokedSocket() = default;
    };

    class SlokedServerSocket {
     public:
        SlokedServerSocket(const SlokedServerSocket &) = delete;
        SlokedServerSocket(SlokedServerSocket &&) = delete;
        virtual ~SlokedServerSocket() = default;
        SlokedServerSocket &operator=(const SlokedServerSocket &) = delete;
        SlokedServerSocket &operator=(SlokedServerSocket &&) = delete;

        virtual bool Valid() = 0;
        virtual void Start() = 0;
        virtual void Close() = 0;
        virtual std::unique_ptr<SlokedSocket> Accept(
            std::chrono::system_clock::duration =
                std::chrono::system_clock::duration::zero()) = 0;
        virtual std::unique_ptr<SlokedIOAwaitable> Awaitable() const = 0;

     protected:
        SlokedServerSocket() = default;
    };

    class SlokedSocketAddress {
     public:
        struct Network {
            std::string host;
            uint16_t port;
        };

        SlokedSocketAddress(Network);
        bool IsNetwork() const;
        const Network &AsNetwork() const;

     private:
        std::variant<Network> address;
    };

    class SlokedSocketFactory {
     public:
        SlokedSocketFactory(const SlokedSocketFactory &) = delete;
        SlokedSocketFactory(SlokedSocketFactory &&) = delete;
        virtual ~SlokedSocketFactory() = default;

        SlokedSocketFactory &operator=(const SlokedSocketFactory &) = delete;
        SlokedSocketFactory &operator=(SlokedSocketFactory &&) = delete;

        virtual std::unique_ptr<SlokedSocket> Connect(
            const SlokedSocketAddress &) = 0;
        virtual std::unique_ptr<SlokedServerSocket> Bind(
            const SlokedSocketAddress &) = 0;

     protected:
        SlokedSocketFactory() = default;
    };
}  // namespace sloked

#endif