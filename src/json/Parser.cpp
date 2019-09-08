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

#include "sloked/json/Parser.h"

namespace sloked {

    JsonDefaultParser::JsonDefaultParser(JsonLexemStream &lexer)
        : lexer(lexer) {
        this->Shift();
    }
    
    std::unique_ptr<JsonASTNode> JsonDefaultParser::Parse() {
        return this->NextElement();
    }

    std::unique_ptr<JsonASTNode> JsonDefaultParser::NextElement() {
        return this->NextValue();
    }

    std::unique_ptr<JsonASTNode> JsonDefaultParser::NextValue() {
        if (this->HasConstant()) {
            return this->NextConstant();
        } else if (this->HasArray()) {
            return this->NextArray();
        } else if (this->HasObject()) {
            return this->NextObject();
        } else {
            return nullptr;
        }
    }

    bool JsonDefaultParser::HasArray() const {
        return this->IsSymbol(JsonLexem::Symbol::OpeningBracket);
    }

    std::unique_ptr<JsonASTNode> JsonDefaultParser::NextArray() {
        if (!this->HasArray()) {
            return nullptr;
        }

        JsonSourcePosition position = this->lexem.value().position;
        this->Shift();
        std::vector<std::shared_ptr<JsonASTNode>> elements;
        while (this->lexem.has_value() && !this->IsSymbol(JsonLexem::Symbol::ClosingBracket)) {
            auto element = this->NextElement();
            if (element) {
                elements.push_back(std::move(element));
            } else {
                return nullptr;
            }

            if (this->IsSymbol(JsonLexem::Symbol::Comma)) {
                this->Shift();
            } else if (!this->IsSymbol(JsonLexem::Symbol::ClosingBracket)) {
                return nullptr;
            }
        }
        
        if (this->IsSymbol(JsonLexem::Symbol::ClosingBracket)) {
            this->Shift();
            return std::make_unique<JsonArrayNode>(std::move(elements), position);
        } else {
            return nullptr;
        }
    }

    bool JsonDefaultParser::HasObject() const {
        return this->IsSymbol(JsonLexem::Symbol::OpeningBrace);
    }

    std::unique_ptr<JsonASTNode> JsonDefaultParser::NextObject() {
        if (!this->HasObject()) {
            return nullptr;
        }

        JsonSourcePosition position = this->lexem.value().position;
        this->Shift();
        std::map<std::string, std::unique_ptr<JsonASTNode>> members;
        while (this->lexem.has_value() && !this->IsSymbol(JsonLexem::Symbol::ClosingBrace)) {
            if (!this->HasString()) {
                return nullptr;
            }
            std::string key = this->NextString();
            if (!this->IsSymbol(JsonLexem::Symbol::Colon)) {
                return nullptr;
            }
            this->Shift();

            auto value = this->NextElement();
            if (value) {
                members.emplace(key, std::move(value));
            } else {
                return nullptr;
            }

            if (this->IsSymbol(JsonLexem::Symbol::Comma)) {
                this->Shift();
            } else if (!this->IsSymbol(JsonLexem::Symbol::ClosingBrace)) {
                return nullptr;
            }
        }

        if (this->IsSymbol(JsonLexem::Symbol::ClosingBrace)) {
            this->Shift();
            return std::make_unique<JsonObjectNode>(std::move(members), position);
        } else {
            return nullptr;
        }
    }

    bool JsonDefaultParser::HasConstant() const {
        return this->lexem.has_value() &&
            this->lexem.value().type != JsonLexem::Type::Symbol;
    }

    std::unique_ptr<JsonASTNode> JsonDefaultParser::NextConstant() {
        if (!this->HasConstant()) {
            return nullptr;
        }

        JsonSourcePosition position = this->lexem.value().position;
        if (this->lexem.value().type == JsonLexem::Type::Boolean) {
            auto value = std::get<bool>(this->lexem.value().value);
            this->Shift();
            return std::make_unique<JsonConstantNode>(value, position);
        } else  if (this->lexem.value().type == JsonLexem::Type::Number) {
            this->Shift();
            return std::make_unique<JsonConstantNode>(position);
        } else if (this->HasNumber()) {
            return this->NextNumber();
        } else if (this->HasInteger()) {
            return this->NextInteger();
        } else if (this->HasString()) {
            return std::make_unique<JsonConstantNode>(this->NextString(), position);
        } else {
            return nullptr;
        }
    }

    bool JsonDefaultParser::HasInteger() const {
        return this->lexem.has_value() &&
            this->lexem.value().type == JsonLexem::Type::Integer;
    }

    std::unique_ptr<JsonASTNode> JsonDefaultParser::NextInteger() {
        if (this->HasInteger()) {
            JsonSourcePosition position = this->lexem.value().position;
            auto value = std::get<int64_t>(this->lexem.value().value);
            this->Shift();
            return std::make_unique<JsonConstantNode>(value, position);
        } else {
            return nullptr;
        }
    }

    bool JsonDefaultParser::HasNumber() const {
        return this->lexem.has_value() &&
            this->lexem.value().type == JsonLexem::Type::Number;
    }

    std::unique_ptr<JsonASTNode> JsonDefaultParser::NextNumber() {
        if (this->HasNumber()) {
            JsonSourcePosition position = this->lexem.value().position;
            auto value = std::get<double>(this->lexem.value().value);
            this->Shift();
            return std::make_unique<JsonConstantNode>(value, position);
        } else {
            return nullptr;
        }
    }

    bool JsonDefaultParser::HasString() const {
        return this->lexem.has_value() &&
            this->lexem.value().type == JsonLexem::Type::String;
    }

    std::string JsonDefaultParser::NextString() {
        if (this->HasString()) {
            auto value = std::get<std::string>(this->lexem.value().value);
            this->Shift();
            return value;
        } else {
            return "";
        }
    }

    void JsonDefaultParser::Shift() {
        this->lexem = this->lexer.Next();
    }

    bool JsonDefaultParser::IsSymbol(JsonLexem::Symbol symbol) const {
        return this->lexem.has_value() &&
            this->lexem.value().type == JsonLexem::Type::Symbol &&
            std::get<JsonLexem::Symbol>(this->lexem.value().value) == symbol;
    }
}