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

#include "sloked/facade/Network.h"
#include "sloked/net/BufferedSocket.h"
#include "sloked/net/CryptoSocket.h"
#include "sloked/net/CompressedSocket.h"

namespace sloked {

    SlokedNetworkFacade::SlokedNetworkFacade(SlokedSocketFactory &base)
        : baseEngine(base) {}

    SlokedSocketFactory &SlokedNetworkFacade::GetBaseEngine() const {
        return this->baseEngine;
    }

    SlokedSocketFactory &SlokedNetworkFacade::GetEngine() const {
        if (this->layers.empty()) {
            return this->baseEngine;
        } else {
            return *this->layers.top();
        }
    }

    bool SlokedNetworkFacade::HasLayers() const {
        return !this->layers.empty();
    }

    void SlokedNetworkFacade::PopLayer() {
        if (this->layers.empty()) {
            throw SlokedError("NetworkFacade: No more layers");
        } else {
            this->layers.pop();
        }
    }

    void SlokedNetworkFacade::BufferingLayer(std::chrono::system_clock::duration timeout, SlokedSchedulerThread &sched) {
        this->layers.push(std::make_unique<SlokedBufferedSocketFactory>(this->GetEngine(), std::move(timeout), sched));
    }
    
    void SlokedNetworkFacade::CompressionLayer(SlokedCompression &compression) {
        this->layers.push(std::make_unique<SlokedCompressedSocketFactory>(this->GetEngine(), compression));
    }

    void SlokedNetworkFacade::EncryptionLayer(SlokedCrypto &crypto, SlokedCrypto::Key &key) {
        this->layers.push(std::make_unique<SlokedCryptoSocketFactory>(this->GetEngine(), crypto, key));
    }
}