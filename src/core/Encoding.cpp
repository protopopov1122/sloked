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

#include "sloked/core/Encoding.h"
#include "sloked/core/Error.h"
#include "sloked/core/Locale.h"
#include <map>

namespace sloked {

    const std::string EncodingIdentifiers::System = "system";
    const std::string EncodingIdentifiers::Utf8 = "utf-8";
    const std::string EncodingIdentifiers::Utf32LE = "utf-32le";

    static std::map<std::string, std::reference_wrapper<const Encoding>> Encodings = {
        { EncodingIdentifiers::Utf8, Encoding::Utf8 },
        { EncodingIdentifiers::Utf32LE, Encoding::Utf32LE }
    };

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

    const Encoding &Encoding::Get(const std::string &id) {
        if (id == EncodingIdentifiers::System) {
            return SlokedLocale::SystemEncoding();
        }
        if (Encodings.count(id) != 0) {
            return Encodings.at(id).get();
        } else {
            throw SlokedError("Unknown encoding " + id);
        }
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