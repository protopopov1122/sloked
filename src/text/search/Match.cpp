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

#include "sloked/text/search/Match.h"
#include "sloked/core/Locale.h"
#include <algorithm>
#include <cstring>

namespace sloked {

    SlokedTextMatcherBase::SlokedTextMatcherBase(const TextBlockView &text, const Encoding &encoding)
        : text(text), conv(encoding, SlokedLocale::SystemEncoding()), current_line(0) {}

    const std::vector<SlokedTextMatcherBase::Result> &SlokedTextMatcherBase::GetResults() const {
        return this->occurences;
    }

    void SlokedTextMatcherBase::Rewind(const TextPosition &position) {
        this->occurences.erase(std::remove_if(this->occurences.begin(), this->occurences.end(), [&](const auto &match) {
            return match.start.line >= position.line;
        }), this->occurences.end());
        this->current_line = position.line;
        this->Search();
    }

    void SlokedTextMatcherBase::Reset() {
        this->occurences.clear();
        this->current_line = 0;
        this->Search();
    }

    SlokedTextRegexMatcher::SlokedTextRegexMatcher(const TextBlockView &text, const Encoding &encoding, bool enable_groups)
        : SlokedTextMatcherBase(text, encoding), enable_groups(enable_groups) {}

    SlokedTextRegexMatcher::SlokedTextRegexMatcher(const TextBlockView &text, const Encoding &encoding)
        : SlokedTextRegexMatcher(text, encoding, true) {}

    void SlokedTextRegexMatcher::Match(const std::string &query, Flags flags) {
        this->occurences.clear();
        this->current_line = 0;
        unsigned int regex_prms = 0;
        if ((flags & SlokedTextMatcher::CaseInsensitive) != 0) {
            regex_prms |= std::regex_constants::icase;
        }
        this->regexp = std::regex(query, static_cast<std::regex::flag_type>(regex_prms));
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
                std::vector<std::string> groups;
                if (this->enable_groups) {
                    for (std::size_t i = 0; i < match.size(); i++) {
                        groups.push_back(this->conv.ReverseConvert(match.str(i)));
                    }
                }
                this->occurences.push_back(Result {
                    pos,
                    length,
                    this->conv.ReverseConvert(match.str()),
                    groups
                });
                offset = pos.column + length;
                line = match.suffix().str();
            }
            this->current_line++;
        });
    }

    SlokedTextPlainMatcher::SlokedTextPlainMatcher(const TextBlockView &text, const Encoding &encoding)
        : SlokedTextRegexMatcher(text, encoding, false) {}

    void SlokedTextPlainMatcher::Match(const std::string &query, Flags flags) {
        std::u32string escapedQuery;
        this->conv.GetDestination().IterateCodepoints(query, [&](auto start, auto length, auto chr) {
            // Escaping raw string
            switch (chr) {
                case U'[':
                case U'\\':
                case U'^':
                case U'$':
                case U'.':
                case U'|':
                case U'?':
                case U'*':
                case U'+':
                case U'(':
                case U')':
                    escapedQuery.append(U"\\");
                    escapedQuery.push_back(chr);
                    break;

                default:
                    escapedQuery.push_back(chr);
                    break;
            }
            return true;
        });
        this->SlokedTextRegexMatcher::Match(this->conv.GetDestination().Encode(escapedQuery), flags);
    }
}