/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#include "sloked/net/BufferedSocket.h"

namespace sloked {

    SlokedBufferedSocket::SlokedBufferedSocket(std::unique_ptr<SlokedSocket> socket, std::chrono::system_clock::duration timeout, SlokedSchedulerThread &sched)
        : socket(std::move(socket)), timeout(std::move(timeout)), sched(sched), task{nullptr} {}

    bool SlokedBufferedSocket::Valid() {
        return this->socket->Valid();
    }

    void SlokedBufferedSocket::Close() {
        this->socket->Close();
    }

    std::size_t SlokedBufferedSocket::Available() {
        return this->socket->Available();
    }

    bool SlokedBufferedSocket::Wait(std::chrono::system_clock::duration timeout) {
        return this->socket->Wait(std::move(timeout));
    }

    std::optional<uint8_t> SlokedBufferedSocket::Read() {
        return this->socket->Read();
    }

    std::vector<uint8_t> SlokedBufferedSocket::Read(std::size_t sz) {
        return this->socket->Read(sz);
    }

    void SlokedBufferedSocket::Write(SlokedSpan<const uint8_t> data) {
        std::unique_lock lock(this->mtx);
        this->buffer.insert(buffer.end(), data.Data(), data.Data() + data.Size());
        if (this->task == nullptr || !this->task->Pending()) {
            this->task = this->sched.Sleep(this->timeout, [this] {
                this->Flush();
            });
        }
    }

    void SlokedBufferedSocket::Write(uint8_t byte) {
        std::unique_lock lock(this->mtx);
        this->buffer.push_back(byte);
        if (this->task == nullptr || !this->task->Pending()) {
            this->task = this->sched.Sleep(this->timeout, [this] {
                this->Flush();
            });
        }
    }

    std::unique_ptr<SlokedIOAwaitable> SlokedBufferedSocket::Awaitable() const {
        return this->socket->Awaitable();
    }

    SlokedSocketEncryption *SlokedBufferedSocket::GetEncryption() {
        return this->socket->GetEncryption();
    }

    void SlokedBufferedSocket::Flush() {
        std::unique_lock lock(this->mtx);
        this->task->Cancel();
        this->socket->Write(SlokedSpan(this->buffer.data(), this->buffer.size()));
        this->buffer.clear();
    }

    SlokedBufferedServerSocket::SlokedBufferedServerSocket(std::unique_ptr<SlokedServerSocket> serverSocket, std::chrono::system_clock::duration timeout, SlokedSchedulerThread &sched)
        : serverSocket(std::move(serverSocket)), timeout(std::move(timeout)), sched(sched) {}

    bool SlokedBufferedServerSocket::Valid() {
        return this->serverSocket->Valid();
    }

    void SlokedBufferedServerSocket::Start() {
        this->serverSocket->Start();
    }

    void SlokedBufferedServerSocket::Close() {
        this->serverSocket->Close();
    }

    std::unique_ptr<SlokedSocket> SlokedBufferedServerSocket::Accept(std::chrono::system_clock::duration timeout) {
        return std::make_unique<SlokedBufferedSocket>(this->serverSocket->Accept(std::move(timeout)), this->timeout, this->sched);
    }

    std::unique_ptr<SlokedIOAwaitable> SlokedBufferedServerSocket::Awaitable() const {
        return this->serverSocket->Awaitable();
    }

    SlokedBufferedSocketFactory::SlokedBufferedSocketFactory(SlokedSocketFactory &socketFactory, std::chrono::system_clock::duration timeout, SlokedSchedulerThread &sched)
        : socketFactory(socketFactory), timeout(std::move(timeout)), sched(sched) {}

    std::unique_ptr<SlokedSocket> SlokedBufferedSocketFactory::Connect(const SlokedSocketAddress &addr) {
        return std::make_unique<SlokedBufferedSocket>(this->socketFactory.Connect(addr), this->timeout, this->sched);
    }

    std::unique_ptr<SlokedServerSocket> SlokedBufferedSocketFactory::Bind(const SlokedSocketAddress &addr) {
        return std::make_unique<SlokedBufferedServerSocket>(this->socketFactory.Bind(addr), this->timeout, this->sched);
    }
}