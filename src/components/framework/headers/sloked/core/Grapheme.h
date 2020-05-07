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

namespace sloked {

    class SlokedGraphemeIterator {
     public:
        virtual ~SlokedGraphemeIterator() = default;
        virtual void Iterate(const Encoding &, std::string_view, std::function<bool(std::size_t, std::size_t, SlokedSpan<const char32_t>)>) const = 0;
    };

    class SlokedGraphemeNullIterator : public SlokedGraphemeIterator {
     public:
        void Iterate(const Encoding &, std::string_view, std::function<bool(std::size_t, std::size_t, SlokedSpan<const char32_t>)>) const final;
    };
}

#endif