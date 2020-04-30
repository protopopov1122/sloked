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

#ifndef SLOKED_JSON_LEXER_H_
#define SLOKED_JSON_LEXER_H_

#include <iosfwd>
#include <optional>
#include <string>
#include <variant>

#include "sloked/json/Position.h"

namespace sloked {

    struct JsonLexem {
        enum class Type { Integer, Number, String, Boolean, Null, Symbol };

        enum class Symbol {
            OpeningBrace,
            ClosingBrace,
            OpeningBracket,
            ClosingBracket,
            Comma,
            Colon
        };

        Type type;
        std::variant<int64_t, double, std::string, bool, Symbol> value;
        JsonSourcePosition position;
    };

    class JsonLexemStream {
     public:
        virtual ~JsonLexemStream() = default;
        virtual std::optional<JsonLexem> Next() = 0;
    };

    class JsonDefaultLexemStream : public JsonLexemStream {
     public:
        JsonDefaultLexemStream(std::istream &, const std::string & = "");
        std::optional<JsonLexem> Next() override;

     private:
        void ReadBuffer();
        void SkipWhitespaces();
        void Shift(std::size_t);

        std::istream &input;
        std::string line_content;
        std::string_view current_line;
        JsonSourcePosition position;
    };
}  // namespace sloked

#endif