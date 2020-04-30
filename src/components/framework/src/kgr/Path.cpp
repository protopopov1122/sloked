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

#include "sloked/kgr/Path.h"

#include "sloked/core/Error.h"
#include "sloked/core/String.h"

namespace sloked {

    std::optional<KgrValue> KgrPath::Traverse(const KgrValue &root,
                                              const SlokedPath &path) {
        if (path.Components().empty()) {
            return root;
        }
        auto entry = path.Components().front();
        if (entry == "..") {
            return {};
        }
        if (entry == ".") {
            if (path.Components().size() > 1) {
                entry = path.Components()[1];
            } else {
                return root;
            }
        }
        auto tail = path.Tail(1);
        switch (root.GetType()) {
            case KgrValueType::Array: {
                std::size_t idx;
                try {
                    idx = std::stoull(entry);
                } catch (const std::invalid_argument &ex) { return {}; }
                if (idx < root.AsArray().Size()) {
                    return Traverse(root.AsArray().At(idx), tail);
                }
            } break;

            case KgrValueType::Object:
                if (root.AsDictionary().Has(entry)) {
                    return Traverse(root.AsDictionary()[entry], tail);
                }
                break;

            default:
                break;
        }
        return {};
    }

    void KgrPath::Assign(KgrValue &root, const SlokedPath &path,
                         const KgrValue &value) {
        if (path.Components().empty()) {
            throw SlokedError("Path: Cannot assign to root");
        }
        auto entry = path.Components().front();
        if (entry == "..") {
            throw SlokedError("Path: Cannot assign to root");
        }
        if (entry == ".") {
            if (path.Components().size() > 1) {
                entry = path.Components()[1];
            } else {
                throw SlokedError("Path: Cannot assign to root");
            }
        }
        auto tail = path.Tail(1);
        if (tail.Components().empty()) {
            if (root.Is(KgrValueType::Object)) {
                root.AsDictionary().Put(entry, value);
            } else if (root.Is(KgrValueType::Array)) {
                if (entry == "end") {
                    root.AsArray().Append(value);
                } else if (starts_with(entry, "ins:")) {
                    entry.erase(entry.begin(), std::next(entry.begin() + 3));
                    root.AsArray().Insert(std::stoull(entry), value);
                } else {
                    root.AsArray().Replace(std::stoull(entry), value);
                }
            }
        } else if (root.Is(KgrValueType::Object)) {
            if (ends_with(entry, "[]")) {
                entry.erase(std::prev(entry.end(), 2), entry.end());
                if (!root.AsDictionary().Has(entry)) {
                    root.AsDictionary().Put(entry, KgrArray{});
                }
            } else if (ends_with(entry, "{}")) {
                entry.erase(std::prev(entry.end(), 2), entry.end());
                if (!root.AsDictionary().Has(entry)) {
                    root.AsDictionary().Put(entry, KgrDictionary{});
                }
            }
            KgrPath::Assign(root.AsDictionary()[entry], tail, value);
        } else if (root.Is(KgrValueType::Array)) {
            std::size_t idx;
            if (ends_with(entry, "[]")) {
                entry.erase(std::prev(entry.end(), 2), entry.end());
                if (entry == "end") {
                    root.AsArray().Append(KgrArray{});
                    idx = root.AsArray().Size() - 1;
                } else if (starts_with(entry, "ins:")) {
                    entry.erase(entry.begin(), std::next(entry.begin() + 3));
                    idx = std::stoull(entry);
                    root.AsArray().Insert(idx, KgrArray{});
                } else {
                    idx = std::stoull(entry);
                }
            } else if (ends_with(entry, "{}")) {
                entry.erase(std::prev(entry.end(), 2), entry.end());
                if (entry == "end") {
                    root.AsArray().Append(KgrDictionary{});
                    idx = root.AsArray().Size() - 1;
                } else if (starts_with(entry, "ins:")) {
                    entry.erase(entry.begin(), std::next(entry.begin() + 3));
                    idx = std::stoull(entry);
                    root.AsArray().Insert(idx, KgrDictionary{});
                } else {
                    idx = std::stoull(entry);
                }
            } else {
                idx = std::stoull(entry);
            }
            KgrPath::Assign(root.AsArray()[idx], tail, value);
        }
    }
}  // namespace sloked