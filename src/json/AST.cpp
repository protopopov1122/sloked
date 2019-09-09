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

#include "sloked/core/Error.h"
#include "sloked/json/AST.h"
#include <iostream>

namespace sloked {

    class JsonASTPrinter : public JsonASTVisitor {
     public:
        JsonASTPrinter(std::ostream &os)
            : out(os) {}

      void Visit(const JsonConstantNode &node) {
        switch (node.GetConstantType()) {
            case JsonConstantNode::DataType::Integer:
                this->out << node.AsInteger();
                break;

            case JsonConstantNode::DataType::Number:
                this->out << node.AsNumber();
                break;

            case JsonConstantNode::DataType::Boolean:
                this->out << (node.AsBoolean() ? "true" : "false");
                break;

            case JsonConstantNode::DataType::String:
                this->out << '\"' << node.AsString() << '\"';
                break;

            case JsonConstantNode::DataType::Null:
                this->out << "null";
        }
      }

      void Visit(const JsonArrayNode &node) override {
        this->out << '[';
        for (std::size_t i = 0; i < node.Length(); i++) {
            if (i > 0) {
                this->out << ',';
            }
            node.At(i).Visit(*this);
        }
        this->out << ']';
      }

      void Visit(const JsonObjectNode &node) override {
        this->out << '{';
        std::size_t count = 0;
        for (const auto &key : node.Keys()) {
            this->out << '\"' << key << "\":";
            node.Get(key).Visit(*this);
            if (++count < node.Keys().size()) {
                this->out << ',';
            }
        }
        this->out << '}';
      }

     private:
        std::ostream &out;
    };

    JsonASTNode::JsonASTNode(Type type, const JsonSourcePosition &position)
        : type(type), position(position) {}

    JsonASTNode::Type JsonASTNode::GetType() const {
        return this->type;
    }

    const JsonSourcePosition &JsonASTNode::GetPosition() const {
        return this->position;
    }

    std::ostream &operator<<(std::ostream &os, const JsonASTNode &node) {
        JsonASTPrinter printer(os);
        node.Visit(printer);
        return os;
    }

    JsonConstantNode::JsonConstantNode(int64_t value, const JsonSourcePosition &position)
        : JsonASTNode(Type::Constant, position), type(DataType::Integer), value(value) {}
    
    JsonConstantNode::JsonConstantNode(double value, const JsonSourcePosition &position)
        : JsonASTNode(Type::Constant, position), type(DataType::Number), value(value) {}
    
    JsonConstantNode::JsonConstantNode(bool value, const JsonSourcePosition &position)
        : JsonASTNode(Type::Constant, position), type(DataType::Boolean), value(value) {}

    JsonConstantNode::JsonConstantNode(std::string_view value, const JsonSourcePosition &position)
        : JsonASTNode(Type::Constant, position), type(DataType::String), value(std::string{value}) {}
    
    JsonConstantNode::JsonConstantNode(const JsonSourcePosition &position)
        : JsonASTNode(Type::Constant, position), type(DataType::Null) {}

    JsonConstantNode::DataType JsonConstantNode::GetConstantType() const {
        return this->type;
    }

    int64_t JsonConstantNode::AsInteger(int64_t defaultValue) const {
        if (this->type == DataType::Integer) {
            return std::get<0>(this->value);
        } else {
            return defaultValue;
        }
    }

    double JsonConstantNode::AsNumber(double defaultValue) const {
        if (this->type == DataType::Number) {
            return std::get<1>(this->value);
        } else {
            return defaultValue;
        }
    }
    
    bool JsonConstantNode::AsBoolean(bool defaultValue) const {
        if (this->type == DataType::Boolean) {
            return std::get<2>(this->value);
        } else {
            return defaultValue;
        }
    }
    
    const std::string &JsonConstantNode::AsString(const std::string &defaultValue) const {
        if (this->type == DataType::String) {
            return std::get<3>(this->value);
        } else {
            return defaultValue;
        }
    }

    void JsonConstantNode::Visit(JsonASTVisitor &visitor) const {
        visitor.Visit(*this);
    }

    JsonArrayNode::JsonArrayNode(std::vector<std::shared_ptr<JsonASTNode>> &&elements, const JsonSourcePosition &position)
        : JsonASTNode(Type::Array, position), elements(std::forward<std::vector<std::shared_ptr<JsonASTNode>>>(elements)) {}
    
    std::size_t JsonArrayNode::Length() const {
        return this->elements.size();
    }

    JsonASTNode &JsonArrayNode::At(std::size_t idx) const {
        if (idx < this->elements.size()) {
            return *this->elements.at(idx);
        } else {
            throw SlokedError("JSON: Out of array bounds");
        }
    }

    void JsonArrayNode::Visit(JsonASTVisitor &visitor) const {
        visitor.Visit(*this);
    }

    JsonObjectNode::JsonObjectNode(std::map<std::string, std::unique_ptr<JsonASTNode>> &&members, const JsonSourcePosition &position)
        : JsonASTNode(Type::Object, position), members(std::forward<std::map<std::string, std::unique_ptr<JsonASTNode>>>(members)) {
        for (const auto &kv : this->members) {
            this->keys.insert(kv.first);
        }
    }

    bool JsonObjectNode::Has(const std::string &key) const {
        return this->members.count(key) != 0;
    }

    JsonASTNode &JsonObjectNode::Get(const std::string &key) const {
        if (this->Has(key)) {
            return *this->members.at(key);
        } else {
            throw SlokedError("JSON: Unknown object key");
        }
    }

    const std::set<std::string> &JsonObjectNode::Keys() const {
        return this->keys;
    }

    void JsonObjectNode::Visit(JsonASTVisitor &visitor) const {
        visitor.Visit(*this);
    }
}