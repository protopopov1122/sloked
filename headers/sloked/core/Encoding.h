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

#ifndef SLOKED_CORE_ENCODING_H_
#define SLOKED_CORE_ENCODING_H_

#include "sloked/Base.h"
#include <functional>
#include <string>
#include <memory>

namespace sloked {

    class Encoding {
     public:
        Encoding(const Encoding &) = delete;
        Encoding(Encoding &&) = delete;
        virtual ~Encoding() = default;
        Encoding &operator=(const Encoding &) = delete;
        Encoding &operator=(Encoding &&) = delete;

        bool operator==(const Encoding &) const;

        virtual std::size_t CodepointCount(std::string_view) const = 0;
        virtual std::pair<std::size_t, std::size_t> GetCodepoint(std::string_view, std::size_t) const = 0;
        virtual std::optional<std::size_t> GetCodepointByOffset(std::string_view, std::size_t) const = 0;
        virtual bool IterateCodepoints(std::string_view, std::function<bool(std::size_t, std::size_t, char32_t)>) const = 0;
        virtual std::string Encode(char32_t) const = 0;
        virtual std::string Encode(std::u32string_view) const = 0;
        virtual std::u32string Decode(std::string_view) const;

        static const Encoding &Get(const std::string &);
        static const Encoding &Utf8;
        static const Encoding &Utf32LE;

     protected:
        Encoding() = default;
    };

    class EncodingConverter {
     public:
        EncodingConverter(const Encoding &, const Encoding &);
        std::string Convert(std::string_view) const;
        std::string ReverseConvert(std::string_view) const;

        const Encoding &GetSource() const;
        const Encoding &GetDestination() const;
        
     private:
        const Encoding &from;
        const Encoding &to;
    };
}

#endif