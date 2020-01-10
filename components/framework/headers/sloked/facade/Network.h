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

#ifndef SLOKED_FACADE_NETWORK_H_
#define SLOKED_FACADE_NETWORK_H_

#include "sloked/net/Socket.h"
#include <stack>

namespace sloked {

    class SlokedNetworkFacade {
     public:
        SlokedNetworkFacade(SlokedSocketFactory &);
        SlokedSocketFactory &GetBaseEngine() const;
        SlokedSocketFactory &GetEngine() const;

        bool HasLayers() const;
        void PopLayer();
        void EncryptionLayer(SlokedCrypto &, SlokedCrypto::Key &);

     private:
        SlokedSocketFactory &baseEngine;
        std::stack<std::unique_ptr<SlokedSocketFactory>> layers;
    };
}

#endif