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

#include "sloked/core/Error.h"

namespace sloked {

    KgrAsyncPipe::KgrAsyncPipe(std::unique_ptr<KgrPipe> pipe)
        : pipe(std::move(pipe)), baseCallback{nullptr} {
        if (this->pipe) {
            this->pipe->SetMessageListener([this] { this->Notify(); });
        }
    }

    KgrAsyncPipe::~KgrAsyncPipe() {
        if (this->pipe) {
            this->pipe->SetMessageListener(nullptr);
        }
    }

    std::unique_ptr<KgrPipe> KgrAsyncPipe::ChangePipe(
        std::unique_ptr<KgrPipe> pipe) {
        std::unique_lock lock(this->mtx);
        auto currentPipe = std::move(this->pipe);
        if (currentPipe) {
            currentPipe->SetMessageListener(nullptr);
        }
        this->pipe = std::move(pipe);
        this->baseCallback = nullptr;
        this->callbacks = {};
        if (this->pipe) {
            this->pipe->SetMessageListener([this] { this->Notify(); });
        }
        return currentPipe;
    }

    bool KgrAsyncPipe::Valid() const {
        std::unique_lock lock(this->mtx);
        return this->pipe != nullptr;
    }

    KgrPipe::Status KgrAsyncPipe::GetStatus() const {
        std::unique_lock lock(this->mtx);
        if (this->pipe) {
            return this->pipe->GetStatus();
        } else {
            throw SlokedError("AsyncPipe: Invalid pipe");
        }
    }

    bool KgrAsyncPipe::Empty() const {
        std::unique_lock lock(this->mtx);
        if (this->pipe) {
            return this->pipe->Empty();
        } else {
            throw SlokedError("AsyncPipe: Invalid pipe");
        }
    }

    std::size_t KgrAsyncPipe::Count() const {
        std::unique_lock lock(this->mtx);
        if (this->pipe) {
            return this->pipe->Count();
        } else {
            throw SlokedError("AsyncPipe: Invalid pipe");
        }
    }

    void KgrAsyncPipe::Close() {
        std::unique_lock lock(this->mtx);
        if (this->pipe) {
            this->pipe->SetMessageListener(nullptr);
            this->pipe->Close();
            this->pipe = nullptr;
        } else {
            throw SlokedError("AsyncPipe: Invalid pipe");
        }
    }

    TaskResult<KgrValue> KgrAsyncPipe::Read() {
        TaskResultSupplier<KgrValue> supplier;
        std::unique_lock lock(this->mtx);
        if (this->pipe == nullptr) {
            throw SlokedError("AsyncPipe: Invalid pipe");
        }
        auto callback = [this, supplier](std::unique_lock<std::mutex> lock) {
            if (!this->pipe->Empty()) {
                this->callbacks.pop();
                lock.unlock();
                supplier.SetResult(this->pipe->Read());
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
        if (this->pipe == nullptr) {
            throw SlokedError("AsyncPipe: Invalid pipe");
        }
        auto callback = [this, supplier,
                         count](std::unique_lock<std::mutex> lock) {
            if (this->pipe->Count() >= count) {
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
        if (this->pipe) {
            this->baseCallback = std::move(callback);
        } else {
            throw SlokedError("AsyncPipe: Invalid pipe");
        }
    }

    void KgrAsyncPipe::Drop(std::size_t count) {
        std::unique_lock lock(this->mtx);
        if (this->pipe) {
            this->pipe->Drop(count);
        } else {
            throw SlokedError("AsyncPipe: Invalid pipe");
        }
    }

    void KgrAsyncPipe::DropAll() {
        std::unique_lock lock(this->mtx);
        if (this->pipe) {
            this->pipe->DropAll();
        } else {
            throw SlokedError("AsyncPipe: Invalid pipe");
        }
    }

    void KgrAsyncPipe::Write(KgrValue &&value) {
        std::unique_lock lock(this->mtx);
        if (this->pipe) {
            this->pipe->Write(std::forward<KgrValue>(value));
        } else {
            throw SlokedError("AsyncPipe: Invalid pipe");
        }
    }

    bool KgrAsyncPipe::SafeWrite(KgrValue &&value) {
        std::unique_lock lock(this->mtx);
        if (this->pipe) {
            return this->pipe->SafeWrite(std::forward<KgrValue>(value));
        } else {
            throw SlokedError("AsyncPipe: Invalid pipe");
        }
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