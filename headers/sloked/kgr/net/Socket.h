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

#ifndef SLOKED_KGR_NET_SOCKET_H_
#define SLOKED_KGR_NET_SOCKET_H_

#include "sloked/Base.h"
#include <vector>
#include <memory>

namespace sloked {

    class KgrSocket {
     public:
        KgrSocket(const KgrSocket &) = delete;
        KgrSocket(KgrSocket &&) = delete;
        virtual ~KgrSocket() = default;
        KgrSocket &operator=(const KgrSocket &&) = delete;
        KgrSocket &operator=(KgrSocket &&) = delete;

        virtual bool IsValid() = 0;
        virtual void Close() = 0;
        virtual std::vector<uint8_t> Read(std::size_t) = 0;
        virtual void Write(const uint8_t *, std::size_t) = 0;

     protected:
        KgrSocket() = default;
    };

    class KgrServerSocket {
     public:
        KgrServerSocket(const KgrServerSocket &) = delete;
        KgrServerSocket(KgrServerSocket &&) = delete;
        virtual ~KgrServerSocket() = default;
        KgrServerSocket &operator=(const KgrServerSocket &&) = delete;
        KgrServerSocket &operator=(KgrServerSocket &&) = delete;

        virtual bool IsValid() = 0;
        virtual void Start() = 0;
        virtual void Close() = 0;
        virtual std::unique_ptr<KgrSocket> Accept() = 0;

     protected:
        KgrServerSocket() = default;
    };

    class KgrSocketFactory {
     public:
        KgrSocketFactory(const KgrSocketFactory &) = delete;
        KgrSocketFactory(KgrSocketFactory &&) = delete;
        virtual ~KgrSocketFactory() = default;

        KgrSocketFactory &operator=(const KgrSocketFactory &) = delete;
        KgrSocketFactory &operator=(KgrSocketFactory &&) = delete;

        virtual std::unique_ptr<KgrSocket> Connect(const std::string &, uint16_t) = 0;
        virtual std::unique_ptr<KgrServerSocket> Bind(const std::string &, uint16_t) = 0;

     protected:
        KgrSocketFactory() = default;
    };
}

#endif