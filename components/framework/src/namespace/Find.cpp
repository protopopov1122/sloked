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

#include "sloked/namespace/Find.h"

namespace sloked {

    SlokedNamespacePlainFind::SlokedNamespacePlainFind(const SlokedNamespace &ns)
        : ns(ns) {}

    std::vector<SlokedPath> SlokedNamespacePlainFind::Query(const SlokedPath &root, const std::string &query, bool include_dirs) const {
        std::vector<SlokedPath> result;
        this->ns.Traverse(root, [&](const std::string &item, auto type) {
            if (item.find(query) != std::string::npos) {
                result.push_back(SlokedPath{item});
            }
        }, include_dirs);
        return result;
    }
}