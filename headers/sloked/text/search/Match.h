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

#include "sloked/core/Position.h"
#include "sloked/core/Encoding.h"
#include "sloked/text/TextBlock.h"
#include <regex>
#include <vector>

namespace sloked {

    class SlokedTextMatcher {
     public:
        struct Result {
            TextPosition start;
            TextPosition length;
            std::string match;
        };

        SlokedTextMatcher(const TextBlockView &, const Encoding &);

        const std::vector<Result> &GetResults() const;
        void Match(const std::string &);
        void Rewind(const TextPosition &);

     private:
        void Search();
        
        const TextBlockView &text;
        EncodingConverter conv;

        TextPosition::Line current_line;
        std::regex regexp;
        std::vector<Result> occurences;
    };
}

#endif