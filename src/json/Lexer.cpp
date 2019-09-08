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

#include "sloked/json/Lexer.h"
#include <regex>
#include <map>
#include <iostream>

namespace sloked {

    static const std::string TrueLiteral = "true";
    static const std::string FalseLiteral = "false";
    static const std::string NullLiteral = "null";
    static const std::regex IntegerRegex(R"(^-?([0]|[1-9][0-9]*))");
    static const std::regex NumberRegex(R"(^-?([0]|[1-9][0-9]*)\.[0-9]+([Ee][+-]?[0-9]+)?)");
    static const std::regex StringRegex(R"(^"(([^"\\])|(\\["\\\/bfnrt])|(\\u[0-9a-fA-F]{4}))*")");
    static const std::map<char, JsonLexem::Symbol> Symbols {
        { '{', JsonLexem::Symbol::OpeningBrace },
        { '}', JsonLexem::Symbol::ClosingBrace },
        { '[', JsonLexem::Symbol::OpeningBracket },
        { ']', JsonLexem::Symbol::ClosingBracket },
        { ',', JsonLexem::Symbol::Comma },
        { ':', JsonLexem::Symbol::Colon }
    };

    static bool starts_with(std::string_view s1, std::string_view s2) {
        if (s1.size() >= s2.size()) {
            for (std::size_t i = 0; i < s2.size(); i++) {
                if (s1[i] != s2[i]) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    }

    JsonDefaultLexemStream::JsonDefaultLexemStream(std::istream &input, const std::string &identifier)
        : input(input), position{identifier, 0, 1} {}

    std::optional<JsonLexem> JsonDefaultLexemStream::Next() {
        std::smatch match;
        this->ReadBuffer();
        this->SkipWhitespaces();
        JsonSourcePosition position = this->position;
        if (starts_with(this->current_line, TrueLiteral)) {
            this->Shift(TrueLiteral.size());
            return JsonLexem{JsonLexem::Type::Boolean, true, position};
        } else if (starts_with(this->current_line, FalseLiteral)) {
            this->Shift(FalseLiteral.size());
            return JsonLexem{JsonLexem::Type::Boolean, false, position};
        } else if (starts_with(this->current_line, NullLiteral)) {
            this->Shift(NullLiteral.size());
            return JsonLexem{JsonLexem::Type::Null, 0l, position};
        } else if (std::regex_search(this->current_line, match, NumberRegex) && !match.empty()) {
            double value = std::stod(match.str());
            this->Shift(match.str().size());
            return JsonLexem{JsonLexem::Type::Number, value, position};
        } else if (std::regex_search(this->current_line, match, IntegerRegex) && !match.empty()) {
            int64_t value = std::stoll(match.str());
            this->Shift(match.str().size());
            return JsonLexem{JsonLexem::Type::Integer, value, position};
        } else if (std::regex_search(this->current_line, match, StringRegex) && !match.empty()) {
            std::string value = match.str().substr(1, match.str().size() - 2);
            this->Shift(match.str().size());
            return JsonLexem{JsonLexem::Type::String, value, position};
        } else if (!this->current_line.empty() && Symbols.count(this->current_line.at(0)) != 0) {
            auto value = Symbols.at(this->current_line.at(0));
            this->Shift(1);
            return JsonLexem{JsonLexem::Type::Symbol, value, position};
        } else {
            return {};
        }
    }

    void JsonDefaultLexemStream::ReadBuffer() {
        while (this->current_line.empty() && this->input.good()) {
            std::getline(this->input, this->current_line);
            this->position.line++;
            this->position.column = 1;
        }
    }

    void JsonDefaultLexemStream::SkipWhitespaces() {
        this->ReadBuffer();
        while (!this->current_line.empty() &&
            (this->current_line.at(0) == '\u0009' ||
            this->current_line.at(0) == '\u000A' ||
            this->current_line.at(0) == '\u000D' ||
            this->current_line.at(0) == '\u0020')) {
            this->Shift(1);
            this->ReadBuffer();
        }
    }

    void JsonDefaultLexemStream::Shift(std::size_t count) {
        this->current_line.erase(0, count);
        this->position.column += count;
    }
}