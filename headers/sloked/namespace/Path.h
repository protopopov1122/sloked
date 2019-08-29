/*
  SPDX-License-Identifier: LGPL-3.0-or-later

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_NAMESPACE_PATH_H_
#define SLOKED_NAMESPACE_PATH_H_

#include "sloked/Base.h"
#include <string>
#include <vector>
#include <functional>

namespace sloked {

    class SlokedPath {
     public:
        using Visitor = std::function<void(const std::string &)>;

        SlokedPath(std::string_view);
        SlokedPath(const char *);
        SlokedPath(const SlokedPath &) = default;
        SlokedPath(SlokedPath &&) = default;

        SlokedPath &operator=(const SlokedPath &) = default;
        SlokedPath &operator=(SlokedPath &&) = default;

        bool IsAbsolute() const;
        const std::string &ToString() const;
        SlokedPath RelativeTo(const SlokedPath &) const;
        void Iterate(Visitor) const;
        const std::vector<std::string> &Components() const;
        SlokedPath GetParent() const;
        SlokedPath GetChild(std::string_view) const;
        bool IsChildOrSelf(const SlokedPath &) const;

        bool operator==(const SlokedPath &) const;

        static constexpr auto Separator = '/';
        static constexpr auto CurrentDir = ".";
        static constexpr auto ParentDir = "..";
        static const SlokedPath Root;

     private:
        SlokedPath() = default;
        SlokedPath &Normalize();

        std::vector<std::string> path;
        std::string literal;
    };
}

#endif