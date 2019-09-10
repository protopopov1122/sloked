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

#include "sloked/core/Encoding.h"

namespace sloked {

    bool Encoding::operator==(const Encoding &other) const {
        return this == &other;
    }

    std::u32string Encoding::Decode(std::string_view view) const {
        std::u32string res;
        this->IterateCodepoints(view, [&](auto start, auto length, auto chr) {
            res.push_back(chr);
            return true;
        });
        return res;
    }

    EncodingConverter::EncodingConverter(const Encoding &from, const Encoding &to)
        : from(from), to(to) {}

    std::string EncodingConverter::Convert(std::string_view str) const {
        if (this->to == this->from) {
            return std::string{str};
        } else {
            return this->to.Encode(this->from.Decode(str));
        }
    }

    std::string EncodingConverter::ReverseConvert(std::string_view str) const {
        if (this->to == this->from) {
            return std::string{str};
        } else {
            return this->from.Encode(this->to.Decode(str));
        }
    }

    const Encoding &EncodingConverter::GetSource() const {
        return this->from;
    }

    const Encoding &EncodingConverter::GetDestination() const {
        return this->to;
    }
}