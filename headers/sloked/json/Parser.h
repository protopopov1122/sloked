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

#ifndef SLOKED_JSON_PARSER_H_
#define SLOKED_JSON_PARSER_H_

#include "sloked/json/AST.h"
#include "sloked/json/Lexer.h"
#include <iosfwd>

namespace sloked {

    class JsonParser {
     public:
        virtual ~JsonParser() = default;
        virtual std::unique_ptr<JsonASTNode> Parse() = 0;
    };

    class JsonDefaultParser : public JsonParser {
     public:
        JsonDefaultParser(JsonLexemStream &);
        std::unique_ptr<JsonASTNode> Parse() override;

     private:
        std::unique_ptr<JsonASTNode> NextElement();
        std::unique_ptr<JsonASTNode> NextValue();
        bool HasArray() const;
        std::unique_ptr<JsonASTNode> NextArray();
        bool HasObject() const;
        std::unique_ptr<JsonASTNode> NextObject();
        bool HasConstant() const;
        std::unique_ptr<JsonASTNode> NextConstant();
        bool HasInteger() const;
        std::unique_ptr<JsonASTNode> NextInteger();
        bool HasNumber() const;
        std::unique_ptr<JsonASTNode> NextNumber();
        bool HasString() const;
        std::string NextString();
        
        void Shift();
        bool IsSymbol(JsonLexem::Symbol) const;

        JsonLexemStream &lexer;
        std::optional<JsonLexem> lexem;
    };
}

#endif