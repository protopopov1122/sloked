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

#ifndef SLOKED_NET_BUFFEREDSOCKET_H_
#define SLOKED_NET_BUFFEREDSOCKET_H_

#include "sloked/net/Socket.h"
#include "sloked/sched/Scheduler.h"

namespace sloked {

    class SlokedBufferedSocket : public SlokedSocket {
     public:
        SlokedBufferedSocket(std::unique_ptr<SlokedSocket>,
                             std::chrono::system_clock::duration,
                             SlokedScheduler &);
        bool Valid() final;
        void Close() final;
        std::size_t Available() final;
        bool Wait(std::chrono::system_clock::duration =
                      std::chrono::system_clock::duration::zero()) final;
        std::optional<uint8_t> Read() final;
        std::vector<uint8_t> Read(std::size_t) final;
        void Write(SlokedSpan<const uint8_t>) final;
        void Write(uint8_t) final;
        std::unique_ptr<SlokedIOAwaitable> Awaitable() const final;
        SlokedSocketEncryption *GetEncryption() final;

     private:
        void Flush();

        std::unique_ptr<SlokedSocket> socket;
        std::chrono::system_clock::duration timeout;
        SlokedScheduler &sched;
        std::shared_ptr<SlokedScheduler::TimerTask> task;
        std::mutex mtx;
        std::vector<uint8_t> buffer;
    };

    class SlokedBufferedServerSocket : public SlokedServerSocket {
     public:
        SlokedBufferedServerSocket(std::unique_ptr<SlokedServerSocket>,
                                   std::chrono::system_clock::duration,
                                   SlokedScheduler &);

        bool Valid() final;
        void Start() final;
        void Close() final;
        std::unique_ptr<SlokedSocket> Accept(
            std::chrono::system_clock::duration =
                std::chrono::system_clock::duration::zero()) final;
        std::unique_ptr<SlokedIOAwaitable> Awaitable() const final;

     private:
        std::unique_ptr<SlokedServerSocket> serverSocket;
        std::chrono::system_clock::duration timeout;
        SlokedScheduler &sched;
    };

    class SlokedBufferedSocketFactory : public SlokedSocketFactory {
     public:
        SlokedBufferedSocketFactory(SlokedSocketFactory &,
                                    std::chrono::system_clock::duration,
                                    SlokedScheduler &);

        std::unique_ptr<SlokedSocket> Connect(
            const SlokedSocketAddress &) final;
        std::unique_ptr<SlokedServerSocket> Bind(
            const SlokedSocketAddress &) final;

     private:
        SlokedSocketFactory &socketFactory;
        std::chrono::system_clock::duration timeout;
        SlokedScheduler &sched;
    };
}  // namespace sloked

#endif