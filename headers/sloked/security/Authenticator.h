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

#ifndef SLOKED_SECUTIRY_AUTHENTICATOR_H_
#define SLOKED_SECUTIRY_AUTHENTICATOR_H_

#include "sloked/core/Crypto.h"
#include "sloked/security/Provider.h"

namespace sloked {

    class SlokedAuthenticator {
     public:
        using Challenge = uint32_t;
        SlokedAuthenticator(SlokedCrypto &, SlokedAuthenticationProvider &, std::string);
        SlokedCrypto &GetCrypto() const;
        SlokedAuthenticationProvider &GetProvider() const;
        const std::string &GetSalt() const;
        Challenge GenerateChallenge() const;
        std::pair<std::string, std::unique_ptr<SlokedCrypto::Cipher>> GenerateResponse(const std::string &, Challenge) const;
        std::unique_ptr<SlokedCrypto::Cipher> VerifyResponse(const std::string &, Challenge, const std::string &) const;

     private:
        std::string GenerateResponse(SlokedCrypto::Cipher &, Challenge) const;

        SlokedCrypto &crypto;
        std::unique_ptr<SlokedCrypto::Random> random;
        SlokedAuthenticationProvider &provider;
        std::string salt;
    };
}

#endif