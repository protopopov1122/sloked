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

#ifndef SLOKED_KGR_NET_POSIXSOCKET_H_
#define SLOKED_KGR_NET_POSIXSOCKET_H_

#include "sloked/kgr/net/Socket.h"

namespace sloked {

    class KgrPosixSocket : public KgrSocket {
     public:
        KgrPosixSocket(int = InvalidSocket);
        KgrPosixSocket(const KgrPosixSocket &) = delete;
        KgrPosixSocket(KgrPosixSocket &&);
        virtual ~KgrPosixSocket();

        KgrPosixSocket &operator=(const KgrPosixSocket &) = delete;
        KgrPosixSocket &operator=(KgrPosixSocket &&);

        void Open(int);

        bool IsValid() final;
        void Close() final;
        std::vector<uint8_t> Read(std::size_t) final;
        void Write(const uint8_t *, std::size_t) final;
        
        static constexpr int InvalidSocket = -1;

     private:
        int socket;
    };

    class KgrPosixServerSocket : public KgrServerSocket {
     public:
        KgrPosixServerSocket(int = InvalidSocket);
        KgrPosixServerSocket(const KgrPosixServerSocket &) = delete;
        KgrPosixServerSocket(KgrPosixServerSocket &&);
        virtual ~KgrPosixServerSocket();

        KgrPosixServerSocket &operator=(const KgrPosixServerSocket &) = delete;
        KgrPosixServerSocket &operator=(KgrPosixServerSocket &&);

        void Open(int);

        bool IsValid() final;
        void Start() final;
        void Close() final;
        std::unique_ptr<KgrSocket> Accept() final;

        static constexpr int InvalidSocket = -1;

     private:
        int socket;
    };

    class KgrPosixSocketFactory : public KgrSocketFactory {
     public:
        KgrPosixSocketFactory() = default;
        std::unique_ptr<KgrSocket> Connect(const std::string &, uint16_t) final;
        std::unique_ptr<KgrServerSocket> Bind(const std::string &, uint16_t) final;
    };
}

#endif