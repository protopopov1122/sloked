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

#include "sloked/kgr/Path.h"

namespace sloked {

    std::optional<KgrValue> KgrPath::Traverse(const KgrValue &root, const SlokedPath &path) {
        if (path.Components().empty()) {
            return root;
        }
        auto front = path.Components().front();
        if (front == "..") {
            return {};
        }
        if (front == ".") {
            if (path.Components().size() > 1) {
                front = path.Components()[1];
            } else {
                return root;
            }
        }
        auto tail = path.Tail(1);
        switch (root.GetType()) {
            case KgrValueType::Array: {
                std::size_t idx;
                try {
                    idx = std::stoull(front);
                } catch (const std::invalid_argument &ex) {
                    return {};
                }
                if (idx < root.AsArray().Size()) {
                    return Traverse(root.AsArray().At(idx), tail);
                }
            } break;

            case KgrValueType::Object:
                if (root.AsDictionary().Has(front)) {
                    return Traverse(root.AsDictionary()[front], tail);
                }
                break;

            default:
                break;
        }
        return {};
    }
}