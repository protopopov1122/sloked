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

#ifndef SLOKED_TEXT_SEARCH_MATCH_H_
#define SLOKED_TEXT_SEARCH_MATCH_H_

#include "sloked/core/Encoding.h"
#include "sloked/text/TextBlock.h"
#include "sloked/text/search/Entry.h"
#include <regex>
#include <vector>

namespace sloked {

    class SlokedTextMatcher {
     public:
         using Result = SlokedSearchEntry;
         using Flags = uint64_t;

         virtual ~SlokedTextMatcher() = default;
         virtual const std::vector<Result> &GetResults() const = 0;
         virtual void Match(const std::string &, Flags = None) = 0;
         virtual void Rewind(const TextPosition &) = 0;

         static constexpr Flags None = 0;
         static constexpr Flags CaseInsensitive = 1;
    };

    class SlokedTextMatcherBase : public SlokedTextMatcher {
     public:
        SlokedTextMatcherBase(const TextBlockView &, const Encoding &);

        const std::vector<Result> &GetResults() const override;
        void Rewind(const TextPosition &) override;

     protected:
        virtual void Search() = 0;

        const TextBlockView &text;
        EncodingConverter conv;

        TextPosition::Line current_line;
        std::vector<Result> occurences;
    };

    class SlokedTextRegexMatcher : public SlokedTextMatcherBase {
     public:
        using SlokedTextMatcherBase::SlokedTextMatcherBase;

        void Match(const std::string &, Flags = None) override;

     protected:
        void Search() override;

        std::regex regexp;
    };

    class SlokedTextPlainMatcher : public SlokedTextRegexMatcher {
     public:
         using SlokedTextRegexMatcher::SlokedTextRegexMatcher;

         void Match(const std::string &, Flags = None) override;
    };
}

#endif