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

#include "sloked/net/Socket.h"

namespace sloked {

    SlokedSocketEncryption *SlokedSocket::GetEncryption() {
        return nullptr;
    }

    SlokedSocketAddress::SlokedSocketAddress(Network addr)
        : address(std::move(addr)) {}

    bool SlokedSocketAddress::IsNetwork() const {
        return this->address.index() == 0;
    }

    const SlokedSocketAddress::Network &SlokedSocketAddress::AsNetwork() const {
        if (this->IsNetwork()) {
            return std::get<0>(this->address);
        } else {
            throw SlokedError("SocketAddress: not a network address");
        }
    }
}  // namespace sloked