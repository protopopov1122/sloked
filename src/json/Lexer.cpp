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

#include "sloked/json/Lexer.h"
#include "sloked/core/String.h"
#include <map>
#include <iostream>

namespace sloked {

    static const std::string TrueLiteral = "true";
    static const std::string FalseLiteral = "false";
    static const std::string NullLiteral = "null";
    static const std::map<char, JsonLexem::Symbol> Symbols {
        { '{', JsonLexem::Symbol::OpeningBrace },
        { '}', JsonLexem::Symbol::ClosingBrace },
        { '[', JsonLexem::Symbol::OpeningBracket },
        { ']', JsonLexem::Symbol::ClosingBracket },
        { ',', JsonLexem::Symbol::Comma },
        { ':', JsonLexem::Symbol::Colon }
    };

    static bool MatchInteger(std::string_view str, std::string &match) {
        match = "";
        if (!str.empty() && str[0] == '-') {
            match.push_back('-');
            str.remove_prefix(1);
        }
        if (!str.empty() && str[0] >= '0' && str[0] <= '9') {
            while (!str.empty() && isdigit(str[0])) {
                match.push_back(str[0]);
                str.remove_prefix(1);
            }
            return true;
        } else {
            return false;
        }
    }

    static bool ishex(char c) {
        return isdigit(c) ||
            c == 'a' || c == 'A' ||
            c == 'b' || c == 'B' ||
            c == 'c' || c == 'C' ||
            c == 'd' || c == 'D' ||
            c == 'e' || c == 'E' ||
            c == 'f' || c == 'F';
    }

    static bool MatchString(std::string_view str, std::string &match) {
        match = "";
        if (str.empty() || str[0] != '\"') {
            return false;
        }
        match.push_back('\"');
        str.remove_prefix(1);
        while (!str.empty() && str[0] != '\"') {
            if (str[0] == '\\') {
                match.push_back('\\');
                str.remove_prefix(1);
                switch (str[0]) {
                    case '\"':
                    case '\\':
                    case '/':
                    case 'b':
                    case 'f':
                    case 'n':
                    case 'r':
                    case 't':
                        match.push_back(str[0]);
                        str.remove_prefix(1);
                        break;

                    case 'u':
                        match.push_back(str[0]);
                        str.remove_prefix(1);
                        if (str.size() < 4 ||
                            !ishex(str[0]) ||
                            !ishex(str[1]) ||
                            !ishex(str[2]) ||
                            !ishex(str[3])) {
                            return false;
                        } else {
                            match += str.substr(0, 4);
                            str.remove_prefix(4);
                        }
                        break;
                    
                    default:
                        return false;
                }
            } else {
                match.push_back(str[0]);
                str.remove_prefix(1);
            }
        }
        if (str.empty()) {
            return false;
        } else {
            match.push_back('\"');
            return true;
        }
    }

    static bool MatchNumber(std::string_view str, std::string &match) {
        if (!MatchInteger(str, match)) {
            return false;
        }
        str.remove_prefix(match.size());
        if (str.empty() || str[0] != '.') {
            return false;
        }
        match.push_back('.');
        str.remove_prefix(1);
        std::string fraction;
        if (!MatchInteger(str, fraction)) {
            return false;
        }
        match.insert(match.end(), fraction.begin(), fraction.end());
        if (str[0] == 'e' || str[0] == 'E') {
            match.push_back(str[0]);
            str.remove_prefix(1);
            std::string exponent;
            if (str[0] == '+') {
                match.push_back(str[0]);
                str.remove_prefix(1);
            }
            if (MatchInteger(str, exponent)) {
                match.insert(match.end(), exponent.begin(), exponent.end());
                return true;
            } else {
                return false;
            }
        } else {
            return true;
        }
    }

    static bool MatchSymbol(std::string_view str) {
        return !str.empty() &&
            (str[0] == '{' ||
            str[0] == '}' ||
            str[0] == '[' ||
            str[0] == ']' ||
            str[0] == ':' ||
            str[0] == ',');
     }

    JsonDefaultLexemStream::JsonDefaultLexemStream(std::istream &input, const std::string &identifier)
        : input(input), position{identifier, 0, 1} {}

    std::optional<JsonLexem> JsonDefaultLexemStream::Next() {
        this->ReadBuffer();
        this->SkipWhitespaces();
        JsonSourcePosition position = this->position;
        std::string strMatch;
        if (this->current_line.empty()) {
            return {};
        }
        if (this->current_line[0] == 't' || this->current_line[0] == 'f' || this->current_line[0] == 'n') {
            if (starts_with(this->current_line, TrueLiteral)) {
                this->Shift(TrueLiteral.size());
                return JsonLexem{JsonLexem::Type::Boolean, true, position};
            } else if (starts_with(this->current_line, FalseLiteral)) {
                this->Shift(FalseLiteral.size());
                return JsonLexem{JsonLexem::Type::Boolean, false, position};
            } else if (starts_with(this->current_line, NullLiteral)) {
                this->Shift(NullLiteral.size());
                return JsonLexem{JsonLexem::Type::Null, 0l, position};
            } else {
                return {};
            }
        } else if (this->current_line[0] == '-' || isdigit(this->current_line[0])) {
            if (MatchNumber(this->current_line, strMatch)) {
                double value = std::stod(strMatch);
                this->Shift(strMatch.size());
                return JsonLexem{JsonLexem::Type::Number, value, position};
            } else if (MatchInteger(this->current_line, strMatch)) {
                int64_t value = std::stoll(strMatch);
                this->Shift(strMatch.size());
                return JsonLexem{JsonLexem::Type::Integer, value, position};
            } else {
                return {};
            }
        } else if (this->current_line[0] == '\"' && MatchString(this->current_line, strMatch)) {
            std::string value = strMatch.substr(1, strMatch.size() - 2);
            this->Shift(strMatch.size());
            return JsonLexem{JsonLexem::Type::String, value, position};
        } else if (MatchSymbol(this->current_line)) {
            auto value = Symbols.at(this->current_line.at(0));
            this->Shift(1);
            return JsonLexem{JsonLexem::Type::Symbol, value, position};
        } else {
            return {};
        }
    }

    void JsonDefaultLexemStream::ReadBuffer() {
        while (this->current_line.empty() && this->input.good()) {
            std::getline(this->input, this->line_content);
            this->current_line = this->line_content;
            this->position.line++;
            this->position.column = 1;
        }
    }

    void JsonDefaultLexemStream::SkipWhitespaces() {
        this->ReadBuffer();
        std::size_t shift = 0;
        while (!this->current_line.empty() &&
            (this->current_line.at(0) == '\u0009' ||
            this->current_line.at(0) == '\u000A' ||
            this->current_line.at(0) == '\u000D' ||
            this->current_line.at(0) == '\u0020')) {
            this->Shift(1);
            if (this->current_line.empty()) {
                this->Shift(shift);
                this->ReadBuffer();
            }
        }
    }

    void JsonDefaultLexemStream::Shift(std::size_t count) {
        this->current_line.remove_prefix(count);
        this->position.column += count;
    }
}