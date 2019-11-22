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

#include "sloked/core/awaitable/Posix.h"
#include "sloked/core/Error.h"

namespace sloked {

    static const int PosixSystemId = 0;
    const intptr_t SlokedPosixAwaitable::PosixIOSystemId = reinterpret_cast<intptr_t>(&PosixSystemId);

    SlokedPosixAwaitable::SlokedPosixAwaitable(int socket)
        : socket(socket) {}

    int SlokedPosixAwaitable::GetSocket() const {
        return this->socket;
    }

    SlokedPosixAwaitable::SystemId SlokedPosixAwaitable::GetSystemId() const {
        return SlokedPosixAwaitable::PosixIOSystemId;
    }


    SlokedIOAwaitable::SystemId SlokedPosixSocketPoll::GetSystemId() const {
        return SlokedPosixAwaitable::PosixIOSystemId;
    }

    std::function<void()> SlokedPosixSocketPoll::Attach(std::unique_ptr<SlokedIOAwaitable> awaitable, std::function<void()> callback) {
        if (awaitable->GetSystemId() != this->GetSystemId()) {
            throw SlokedError("SlokedIOPoll: Unsupported awaitable type");
        }
        auto socket = static_cast<SlokedPosixAwaitable *>(awaitable.get())->GetSocket();
        std::unique_lock lock(this->mtx);
        this->sockets.emplace(socket, std::move(callback));
        return [this, socket] {
            std::unique_lock lock(this->mtx);
            if (this->sockets.count(socket) != 0) {
                this->sockets.erase(socket);
            }
        };
    }

    void SlokedPosixSocketPoll::Await(long timeout) {
        struct timeval tv;
        fd_set rfds;
        FD_ZERO(&rfds);
        tv.tv_sec = 0;
        tv.tv_usec = timeout * 1000;

        int max_socket = std::numeric_limits<int>::min();
        std::unique_lock lock(this->mtx);
        if (this->sockets.empty()) {
            return;
        }

        for (const auto &kv : this->sockets) {
            FD_SET(kv.first, &rfds);
            if (kv.first > max_socket) {
                max_socket = kv.first;
            }
        }
        lock.unlock();

        int res = select(max_socket + 1, &rfds, nullptr, nullptr, timeout > 0 ? &tv : nullptr);
        if (res > 0) {
            std::vector<std::function<void()>> callbacks;
            lock.lock();
            for (const auto &kv : this->sockets) {
                if (FD_ISSET(kv.first, &rfds)) {
                    callbacks.push_back(kv.second);
                }
            }
            lock.unlock();
            for (const auto &callback : callbacks) {
                callback();
            }
        }
    }
}