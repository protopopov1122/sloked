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

#ifndef SLOKED_SECURITY_CREDENTIALSTORAGE_H_
#define SLOKED_SECURITY_CREDENTIALSTORAGE_H_

#include <functional>
#include <string>

#include "sloked/core/Crypto.h"

namespace sloked {

    class SlokedCredentialStorage {
     public:
        class Account {
         public:
            using Callback = std::function<void()>;
            virtual ~Account() = default;
            virtual std::string GetIdentifier() const = 0;
            virtual std::string GetPassword() const = 0;
            virtual std::unique_ptr<SlokedCrypto::Key> DeriveKey(
                std::size_t, const std::string &) const = 0;
            virtual Callback Watch(Callback) = 0;
        };

        virtual ~SlokedCredentialStorage() = default;
        virtual bool Has(const std::string &) const = 0;
        virtual std::shared_ptr<Account> GetByName(
            const std::string &) const = 0;
    };
}  // namespace sloked

#endif