/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#include "sloked/json/Parser.h"
#include "sloked/core/Error.h"
#include <sstream>

namespace sloked {

    JsonDefaultParser::JsonDefaultParser(JsonLexemStream &lexer)
        : lexer(lexer) {
        this->Shift();
    }
    
    std::unique_ptr<JsonASTNode> JsonDefaultParser::Parse() {
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
            this->ThrowError("Expected constant|array|object");
        }
    }

    bool JsonDefaultParser::HasArray() const {
        return this->IsSymbol(JsonLexem::Symbol::OpeningBracket);
    }

    std::unique_ptr<JsonASTNode> JsonDefaultParser::NextArray() {
        if (!this->HasArray()) {
            this->ThrowError("Internal error");
        }

        JsonSourcePosition position = this->lexem.value().position;
        this->Shift();
        std::vector<std::shared_ptr<JsonASTNode>> elements;
        while (this->lexem.has_value() && !this->IsSymbol(JsonLexem::Symbol::ClosingBracket)) {
            auto element = this->NextValue();
            if (element) {
                elements.push_back(std::move(element));
            } else {
                this->ThrowError("Internal error");
            }

            if (this->IsSymbol(JsonLexem::Symbol::Comma)) {
                this->Shift();
            } else if (!this->IsSymbol(JsonLexem::Symbol::ClosingBracket)) {
                this->ThrowError("Expected \']\'|\',\'");
            }
        }
        
        if (this->IsSymbol(JsonLexem::Symbol::ClosingBracket)) {
            this->Shift();
            return std::make_unique<JsonArrayNode>(std::move(elements), position);
        } else {
            this->ThrowError("Expected \']\'");
        }
    }

    bool JsonDefaultParser::HasObject() const {
        return this->IsSymbol(JsonLexem::Symbol::OpeningBrace);
    }

    std::unique_ptr<JsonASTNode> JsonDefaultParser::NextObject() {
        if (!this->HasObject()) {
            this->ThrowError("Internal error");
        }

        JsonSourcePosition position = this->lexem.value().position;
        this->Shift();
        std::map<std::string, std::unique_ptr<JsonASTNode>> members;
        while (this->lexem.has_value() && !this->IsSymbol(JsonLexem::Symbol::ClosingBrace)) {
            if (!this->HasString()) {
                this->ThrowError("Expected string literal");
            }
            std::string key = this->NextString();
            if (!this->IsSymbol(JsonLexem::Symbol::Colon)) {
                this->ThrowError("Expected \':\'");
            }
            this->Shift();

            auto value = this->NextValue();
            if (value) {
                members.emplace(key, std::move(value));
            } else {
                this->ThrowError("Internal error");
            }

            if (this->IsSymbol(JsonLexem::Symbol::Comma)) {
                this->Shift();
            } else if (!this->IsSymbol(JsonLexem::Symbol::ClosingBrace)) {
                this->ThrowError("Expected \'}\'|\',\'");
            }
        }

        if (this->IsSymbol(JsonLexem::Symbol::ClosingBrace)) {
            this->Shift();
            return std::make_unique<JsonObjectNode>(std::move(members), position);
        } else {
            this->ThrowError("Expected \'}\'");
        }
    }

    bool JsonDefaultParser::HasConstant() const {
        return this->lexem.has_value() &&
            this->lexem.value().type != JsonLexem::Type::Symbol;
    }

    std::unique_ptr<JsonASTNode> JsonDefaultParser::NextConstant() {
        if (!this->HasConstant()) {
            this->ThrowError("Internal error");
        }

        JsonSourcePosition position = this->lexem.value().position;
        if (this->HasString()) {
            return std::make_unique<JsonConstantNode>(this->NextString(), position);
        } else if (this->HasInteger()) {
            return this->NextInteger();
        } else if (this->lexem.value().type == JsonLexem::Type::Boolean) {
            auto value = std::get<bool>(this->lexem.value().value);
            this->Shift();
            return std::make_unique<JsonConstantNode>(value, position);
        } else  if (this->lexem.value().type == JsonLexem::Type::Null) {
            this->Shift();
            return std::make_unique<JsonConstantNode>(position);
        } else if (this->HasNumber()) {
            return this->NextNumber();
        } else {
            this->ThrowError("Expected integer|number|boolean|string");
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
            this->ThrowError("Internal error");
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
            this->ThrowError("Internal error");
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
            this->ThrowError("Internal error");
        }
    }

    void JsonDefaultParser::Shift() {
        constexpr std::size_t BufferSize = 1024;
        if (this->buffer.empty()) {
            while (this->buffer.size() < BufferSize) {
                auto lexem = this->lexer.Next();
                if (lexem.has_value()) {
                    this->buffer.push(std::move(lexem.value()));
                } else {
                    break;
                }
            }
        }
        if (!this->buffer.empty()) {
            this->lexem = std::move(this->buffer.front());
            this->buffer.pop();
        } else {
            this->lexem = {};
        }
    }

    bool JsonDefaultParser::IsSymbol(JsonLexem::Symbol symbol) const {
        return this->lexem.has_value() &&
            this->lexem.value().type == JsonLexem::Type::Symbol &&
            std::get<JsonLexem::Symbol>(this->lexem.value().value) == symbol;
    }

    [[noreturn]] void JsonDefaultParser::ThrowError(std::string_view msg) const {
        std::stringstream ss;
        ss << "JSON Parser: Error \'" << msg << "\'";
        if (this->lexem.has_value()) {
            ss << " at ";
            auto pos = this->lexem.value().position;
            if (!pos.identifier.empty()) {
                ss << pos.identifier << ":";
            }
            ss << pos.line << ":" << pos.column;
        }
        throw SlokedError(ss.str());
    }
}