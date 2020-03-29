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

    static bool ContainsPrefix(const SlokedPath &name,
                               const std::vector<SlokedPath> &prefixes) {
        for (const auto &prefix : prefixes) {
            if (prefix.IsParent(name)) {
                return true;
            }
        }
        return false;
    }

    SlokedNamedWhitelist::SlokedNamedWhitelist(std::vector<SlokedPath> prefixes)
        : prefixes(std::move(prefixes)) {}

    bool SlokedNamedWhitelist::IsAllowed(const SlokedPath &name) const {
        return ContainsPrefix(
            name.IsAbsolute() ? name : name.RelativeTo(name.Root()),
            this->prefixes);
    }

    std::unique_ptr<SlokedNamedWhitelist> SlokedNamedWhitelist::Make(
        std::vector<SlokedPath> prefixes) {
        return std::make_unique<SlokedNamedWhitelist>(std::move(prefixes));
    }

    SlokedNamedBlacklist::SlokedNamedBlacklist(std::vector<SlokedPath> prefixes)
        : prefixes(prefixes) {}

    bool SlokedNamedBlacklist::IsAllowed(const SlokedPath &name) const {
        return !ContainsPrefix(
            name.IsAbsolute() ? name : name.RelativeTo(name.Root()),
            this->prefixes);
    }

    std::unique_ptr<SlokedNamedBlacklist> SlokedNamedBlacklist::Make(
        std::vector<SlokedPath> prefixes) {
        return std::make_unique<SlokedNamedBlacklist>(std::move(prefixes));
    }
}  // namespace sloked