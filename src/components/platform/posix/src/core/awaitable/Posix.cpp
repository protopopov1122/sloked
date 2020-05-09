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

#include "sloked/core/awaitable/Posix.h"

#include <vector>

#include "sloked/core/Error.h"
#include "sloked/core/posix/Time.h"

namespace sloked {

    static const int PosixSystemId = 0;
    const intptr_t SlokedPosixAwaitable::PosixIOSystemId =
        reinterpret_cast<intptr_t>(&PosixSystemId);

    SlokedPosixAwaitable::SlokedPosixAwaitable(int socket) : socket(socket) {}

    int SlokedPosixAwaitable::GetSocket() const {
        return this->socket;
    }

    SlokedPosixAwaitablePoll::SlokedPosixAwaitablePoll() : max_socket{std::numeric_limits<int>::min()} {
        fd_set rfds;
        FD_ZERO(&rfds);
        this->descriptors.store(rfds);
    }

    SlokedPosixAwaitable::SystemId SlokedPosixAwaitable::GetSystemId() const {
        return SlokedPosixAwaitable::PosixIOSystemId;
    }

    SlokedIOAwaitable::SystemId SlokedPosixAwaitablePoll::GetSystemId() const {
        return SlokedPosixAwaitable::PosixIOSystemId;
    }

    bool SlokedPosixAwaitablePoll::Empty() const {
        return this->max_socket.load() == std::numeric_limits<int>::min();
    }

    std::function<void()> SlokedPosixAwaitablePoll::Attach(
        std::unique_ptr<SlokedIOAwaitable> awaitable,
        std::function<void()> callback) {
        if (awaitable->GetSystemId() != this->GetSystemId()) {
            throw SlokedError("SlokedIOPoll: Unsupported awaitable type");
        }
        auto socket =
            static_cast<SlokedPosixAwaitable *>(awaitable.get())->GetSocket();
        std::unique_lock lock(this->mtx);
        this->sockets.emplace(socket, std::move(callback));
        auto rfds = this->descriptors.load();
        FD_SET(socket, &rfds);
        this->descriptors.store(rfds);
        if (socket > this->max_socket.load()) {
            this->max_socket.store(socket);
        }
        return [this, socket] {
            std::unique_lock lock(this->mtx);
            if (this->sockets.count(socket) != 0) {
                this->sockets.erase(socket);
                auto rfds = this->descriptors.load();
                FD_CLR(socket, &rfds);
                this->descriptors.store(rfds);
                this->max_socket = std::numeric_limits<int>::min();
                for (const auto &kv : this->sockets) {
                    if (kv.first > this->max_socket.load()) {
                        max_socket.store(kv.first);
                    }
                }
            }
        };
    }

    void SlokedPosixAwaitablePoll::Await(
        std::chrono::system_clock::duration timeout) {
        struct timeval tv;
        auto rfds = this->descriptors.load();
        DurationToTimeval(timeout, tv);

        int res = select(this->max_socket.load() + 1, &rfds, nullptr, nullptr,
                         timeout > std::chrono::system_clock::duration::zero()
                             ? &tv
                             : nullptr);
        if (res > 0) {
            std::unique_lock lock(this->mtx);
            callbacks.clear();
            for (const auto &kv : this->sockets) {
                if (FD_ISSET(kv.first, &rfds)) {
                    callbacks.emplace_back(kv.second);
                }
            }
            lock.unlock();
            for (const auto &callback : callbacks) {
                callback();
            }
        }
    }
}  // namespace sloked
