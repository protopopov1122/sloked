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

#include "sloked/kgr/Pipe.h"

namespace sloked {

    KgrAsyncPipe::KgrAsyncPipe(KgrPipe &pipe)
        : pipe(pipe), baseCallback{nullptr} {
        this->pipe.SetMessageListener([this] { this->Notify(); });
    }

    KgrAsyncPipe::~KgrAsyncPipe() {
        this->pipe.SetMessageListener(nullptr);
    }

    KgrPipe::Status KgrAsyncPipe::GetStatus() const {
        return this->pipe.GetStatus();
    }

    bool KgrAsyncPipe::Empty() const {
        return this->pipe.Empty();
    }

    std::size_t KgrAsyncPipe::Count() const {
        return this->pipe.Count();
    }

    void KgrAsyncPipe::Close() {
        this->pipe.Close();
    }

    TaskResult<KgrValue> KgrAsyncPipe::Read() {
        TaskResultSupplier<KgrValue> supplier;
        std::unique_lock lock(this->mtx);
        auto callback = [this, supplier](std::unique_lock<std::mutex> lock) {
            if (!this->pipe.Empty()) {
                this->callbacks.pop();
                lock.unlock();
                supplier.SetResult(this->pipe.Read());
            }
        };
        if (this->callbacks.empty()) {
            this->callbacks.push(callback);
            callback(std::move(lock));
        } else {
            this->callbacks.push(callback);
            this->callbacks.front()(std::move(lock));
        }
        return supplier.Result();
    }

    TaskResult<void> KgrAsyncPipe::Wait(std::size_t count) {
        TaskResultSupplier<void> supplier;
        std::unique_lock lock(this->mtx);
        auto callback = [this, supplier,
                         count](std::unique_lock<std::mutex> lock) {
            if (this->pipe.Count() >= count) {
                this->callbacks.pop();
                lock.unlock();
                supplier.SetResult();
            }
        };
        if (this->callbacks.empty()) {
            this->callbacks.push(callback);
            callback(std::move(lock));
        } else {
            this->callbacks.push(callback);
            this->callbacks.front()(std::move(lock));
        }
        return supplier.Result();
    }

    void KgrAsyncPipe::SetMessageListener(std::function<void()> callback) {
        std::unique_lock lock(this->mtx);
        this->baseCallback = std::move(callback);
    }

    void KgrAsyncPipe::Drop(std::size_t count) {
        this->pipe.Drop(count);
    }

    void KgrAsyncPipe::DropAll() {
        this->pipe.DropAll();
    }

    void KgrAsyncPipe::Write(KgrValue &&value) {
        this->pipe.Write(std::forward<KgrValue>(value));
    }

    bool KgrAsyncPipe::WriteNX(KgrValue &&value) {
        return this->pipe.WriteNX(std::forward<KgrValue>(value));
    }

    void KgrAsyncPipe::Notify() {
        std::unique_lock lock(this->mtx);
        if (this->callbacks.empty()) {
            auto callback = this->baseCallback;
            lock.unlock();
            if (callback) {
                callback();
            }
        } else {
            auto callback = this->callbacks.front();
            callback(std::move(lock));
        }
    }
}  // namespace sloked