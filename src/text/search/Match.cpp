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

#include "sloked/text/search/Match.h"
#include "sloked/core/Locale.h"
#include <algorithm>

namespace sloked {

    SlokedTextMatcherBase::SlokedTextMatcherBase(const TextBlockView &text, const Encoding &encoding)
        : text(text), conv(encoding, SlokedLocale::SystemEncoding()), current_line(0) {}

    const std::vector<SlokedTextMatcherBase::Result> &SlokedTextMatcherBase::GetResults() const {
        return this->occurences;
    }

    void SlokedTextMatcherBase::Rewind(const TextPosition &position) {
        TextPosition start{position.line, 0};
        std::remove_if(this->occurences.begin(), this->occurences.end(), [&](const auto &match) {
            return match.start >= start;
        });
        this->current_line = position.line;
        this->Search();
    }

    void SlokedTextPlainMatcher::Match(const std::string &query) {
        this->occurences.clear();
        this->current_line = 0;
        this->query = query;
        this->Search();
    }

    void SlokedTextPlainMatcher::Search() {
        this->text.Visit(this->current_line, this->text.GetLastLine() - this->current_line + 1, [&](const auto lineView) {
            std::size_t match;
            const std::string line{this->conv.Convert(lineView)};
            std::size_t offset = 0;
            const TextPosition::Column length = this->conv.GetDestination().CodepointCount(this->query);
            const std::string revQuery{this->conv.ReverseConvert(this->query)};
            while ((match = line.find(this->query, offset)) != std::string::npos) {
                TextPosition pos {
                    this->current_line,
                    static_cast<TextPosition::Column>(this->conv.GetDestination().GetCodepointByOffset(line, match).value_or(0))
                };
                this->occurences.push_back(Result {
                    pos,
                    length,
                    revQuery,
                    {}
                });
                offset = pos.column + length;
            }
            this->current_line++;
        });
    }

    void SlokedTextRegexMatcher::Match(const std::string &query) {
        this->occurences.clear();
        this->current_line = 0;
        this->regexp = std::regex(query);
        this->Search();
    }

    void SlokedTextRegexMatcher::Search() {
        this->text.Visit(this->current_line, this->text.GetLastLine() - this->current_line + 1, [&](const auto lineView) {
            std::smatch match;
            std::string line{this->conv.Convert(lineView)};
            std::size_t offset = 0;
            while (std::regex_search(line, match, this->regexp)) {
                TextPosition pos {
                    this->current_line,
                    static_cast<TextPosition::Column>(this->conv.GetDestination().GetCodepointByOffset(line, match.position()).value_or(0) + offset)
                };
                TextPosition::Column length = this->conv.GetDestination().CodepointCount(match.str());
                this->occurences.push_back(Result {
                    pos,
                    length,
                    this->conv.ReverseConvert(match.str()),
                    {}
                });
                offset = pos.column + length;
                line = match.suffix().str();
            }
            this->current_line++;
        });
    }
}