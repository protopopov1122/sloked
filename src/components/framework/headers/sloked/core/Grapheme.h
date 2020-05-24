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

#ifndef SLOKED_CORE_GRAPHEME_H_
#define SLOKED_CORE_GRAPHEME_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/Span.h"
#include <vector>
#include <optional>

namespace sloked {

    class SlokedGraphemeEnumerator {
     public:
        using IteratorCallback = std::function<bool(std::size_t, std::size_t, SlokedSpan<const char32_t>)>;
        struct Grapheme {
            std::size_t offset;
            std::size_t length;
            std::vector<char32_t> codepoints;
        };

        virtual ~SlokedGraphemeEnumerator() = default;
        virtual std::size_t Count(const Encoding &, std::string_view) const = 0;
        virtual std::optional<Grapheme> Get(const Encoding &, std::string_view, std::size_t) const = 0;
        virtual bool Iterate(const Encoding &, std::string_view, IteratorCallback) const = 0;
    };

    class SlokedGraphemeNullEnumerator : public SlokedGraphemeEnumerator {
     public:
        std::size_t Count(const Encoding &, std::string_view) const final;
        std::optional<Grapheme> Get(const Encoding &, std::string_view, std::size_t) const final;
        bool Iterate(const Encoding &, std::string_view, IteratorCallback) const final;
    };
}

#endif
