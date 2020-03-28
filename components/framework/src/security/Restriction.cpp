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

#include "sloked/security/Restriction.h"

#include "sloked/core/String.h"

namespace sloked {

    static bool ContainsPrefix(const std::string &name,
                               const std::vector<std::string> &prefixes) {
        for (const auto &prefix : prefixes) {
            if (starts_with(name, prefix)) {
                return true;
            }
        }
        return false;
    }

    SlokedNamedWhitelist::SlokedNamedWhitelist(
        std::vector<std::string> prefixes)
        : prefixes(std::move(prefixes)) {}

    bool SlokedNamedWhitelist::IsAllowed(const std::string &name) const {
        return ContainsPrefix(name, this->prefixes);
    }

    std::unique_ptr<SlokedNamedWhitelist> SlokedNamedWhitelist::Make(
        std::vector<std::string> prefixes) {
        return std::make_unique<SlokedNamedWhitelist>(std::move(prefixes));
    }

    SlokedNamedBlacklist::SlokedNamedBlacklist(
        std::vector<std::string> prefixes)
        : prefixes(prefixes) {}

    bool SlokedNamedBlacklist::IsAllowed(const std::string &name) const {
        return !ContainsPrefix(name, this->prefixes);
    }

    std::unique_ptr<SlokedNamedBlacklist> SlokedNamedBlacklist::Make(
        std::vector<std::string> prefixes) {
        return std::make_unique<SlokedNamedBlacklist>(std::move(prefixes));
    }
}  // namespace sloked