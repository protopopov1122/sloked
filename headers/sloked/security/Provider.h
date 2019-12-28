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

#ifndef SLOKED_SECURITY_PROVIDER_H_
#define SLOKED_SECURITY_PROVIDER_H_

#include "sloked/core/Crypto.h"
#include <string>
#include <functional>

namespace sloked {

    class SlokedCredentialProvider {
     public:
        class Account {
         public:
            using Callback = std::function<void()>;
            virtual ~Account() = default;
            virtual std::string GetName() const = 0;
            virtual std::string GetCredentials() const = 0;
            virtual std::unique_ptr<SlokedCrypto::Key> DeriveKey(const std::string &) const = 0;
            virtual Callback Watch(Callback) = 0;
        };

        virtual ~SlokedCredentialProvider() = default;
        virtual bool Has(const std::string &) const = 0;
        virtual std::weak_ptr<Account> GetByName(const std::string &) const = 0;
    };
}

#endif