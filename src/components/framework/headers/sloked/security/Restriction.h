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

#ifndef SLOKED_SECURITY_RESTRICTION_H_
#define SLOKED_SECURITY_RESTRICTION_H_

#include <memory>
#include <string>
#include <vector>

#include "sloked/namespace/Path.h"

namespace sloked {

    class SlokedNamedRestrictions {
     public:
        virtual ~SlokedNamedRestrictions() = default;
        virtual bool IsAllowed(const SlokedPath &) const = 0;
    };

    class SlokedNamedWhitelist : public SlokedNamedRestrictions {
     public:
        SlokedNamedWhitelist(std::vector<SlokedPath> = {});
        bool IsAllowed(const SlokedPath &) const final;
        static std::unique_ptr<SlokedNamedWhitelist> Make(
            std::vector<SlokedPath>);

     private:
        std::vector<SlokedPath> prefixes;
    };

    class SlokedNamedBlacklist : public SlokedNamedRestrictions {
     public:
        SlokedNamedBlacklist(std::vector<SlokedPath> = {});
        bool IsAllowed(const SlokedPath &) const final;
        static std::unique_ptr<SlokedNamedBlacklist> Make(
            std::vector<SlokedPath>);

     private:
        std::vector<SlokedPath> prefixes;
    };

    class SlokedNamedRestrictionTarget {
     public:
        virtual ~SlokedNamedRestrictionTarget() = default;
        virtual void SetAccessRestrictions(
            std::shared_ptr<SlokedNamedRestrictions>) = 0;
        virtual void SetModificationRestrictions(
            std::shared_ptr<SlokedNamedRestrictions>) = 0;
    };

    class SlokedNamedRestrictionAuthority {
     public:
        class Account {
         public:
            virtual ~Account() = default;
            virtual std::shared_ptr<SlokedNamedRestrictions>
                GetAccessRestrictions() const = 0;
            virtual std::shared_ptr<SlokedNamedRestrictions>
                GetModificationRestrictiions() const = 0;
        };

        virtual ~SlokedNamedRestrictionAuthority() = default;
        virtual std::shared_ptr<Account> GetRestrictionsByName(
            const std::string &) = 0;
        virtual std::shared_ptr<Account> GetDefaultRestrictions() = 0;
    };
}  // namespace sloked

#endif