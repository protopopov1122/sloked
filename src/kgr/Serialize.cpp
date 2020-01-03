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

#include "sloked/kgr/Serialize.h"
#include "sloked/json/Parser.h"
#include "sloked/core/Error.h"
#include <sstream>

namespace sloked {

    static const JsonSourcePosition DefaultPosition{"", 0, 0};

    static void unescape(std::string_view src, std::string &dst) {
        while (!src.empty()) {
            if (src[0] == '\\') {
                if (src.size() == 1) {
                    throw SlokedError("JsonSerializer: Unescaping error");
                }
                src.remove_prefix(1);
                switch (src[0]) {
                    case '\"':
                    case '/':
                    case '\\':
                        dst.push_back(src[0]);
                        break;

                    case 'b':
                        dst.push_back('\b');
                        break;

                    case 'f':
                        dst.push_back('\f');
                        break;
                        
                    case 'n':
                        dst.push_back('\n');
                        break;
                        
                    case 'r':
                        dst.push_back('\r');
                        break;
                        
                    case 't':
                        dst.push_back('\t');
                        break;

                    default:
                        throw SlokedError("JsonSerializer: Unescaping error");
                }
            } else {
                dst.push_back(src[0]);
            }
            src.remove_prefix(1);
        }
    }    

    static void escape(std::string_view src, std::string &dst) {
        while (!src.empty()) {
            switch (src[0]) {
                case '\"':
                    dst.append("\\\"");
                    break;

                case '/':
                    dst.append("\\/");
                    break;

                case '\\':
                    dst.append("\\\\");
                    break;

                case '\b':
                    dst.append("\\b");
                    break;

                case '\f':
                    dst.append("\\f");
                    break;
                    
                case '\n':
                    dst.append("\\n");
                    break;
                    
                case '\r':
                    dst.append("\\r");
                    break;
                    
                case '\t':
                    dst.append("\\t");
                    break;

                default:
                    dst.push_back(src[0]);
                    break;
            }
            src.remove_prefix(1);
        }
    }

    class KgrJsonDeserializeVisitor : public JsonASTVisitor<KgrValue> {
     public:
        KgrValue Visit(const JsonConstantNode &value) override {
            switch (value.GetConstantType()) {
                case JsonConstantNode::DataType::Null:
                    return KgrValue();

                case JsonConstantNode::DataType::Integer:
                    return KgrValue(value.AsInteger());

                case JsonConstantNode::DataType::Number:
                    return KgrValue(value.AsNumber());

                case JsonConstantNode::DataType::Boolean:
                    return KgrValue(value.AsBoolean());

                case JsonConstantNode::DataType::String: {
                    std::string res;
                    unescape(value.AsString(), res);
                    return KgrValue(std::move(res));
                }
            }
            return {};
        }

        KgrValue Visit(const JsonArrayNode &value) override {
            std::vector<KgrValue> array;
            for (std::size_t i = 0; i < value.Length(); i++) {
                array.push_back(value.At(i).Visit(*this));
            }
            return KgrValue(std::move(array));
        }

        KgrValue Visit(const JsonObjectNode &value) override {
            std::map<std::string, KgrValue> object;
            for (const auto &key : value.Keys()) {
                object[key] = value.Get(key).Visit(*this);
            }
            return KgrValue(std::move(object));
        }
    };

    KgrJsonSerializer::Blob KgrJsonSerializer::Serialize(const KgrValue &value) const {
        std::stringstream ss;
        auto node = this->SerializeValue(value);
        ss << *node;
        return ss.str();
    }

    KgrValue KgrJsonSerializer::Deserialize(const Blob &blob) const {
        std::stringstream ss(blob);
        JsonDefaultLexemStream lexer(ss);
        JsonDefaultParser parser(lexer);
        auto node = parser.Parse();
        return this->DeserializeValue(*node);
    }

    KgrValue KgrJsonSerializer::Deserialize(std::istream &is) const {
        JsonDefaultLexemStream lexer(is);
        JsonDefaultParser parser(lexer);
        auto node = parser.Parse();
        return this->DeserializeValue(*node);
    }

    std::unique_ptr<JsonASTNode> KgrJsonSerializer::SerializeValue(const KgrValue &value) const {
        switch (value.GetType()) {
            case KgrValueType::Null:
                return std::make_unique<JsonConstantNode>(DefaultPosition);

            case KgrValueType::Integer:
                return std::make_unique<JsonConstantNode>(value.AsInt(), DefaultPosition);

            case KgrValueType::Number:
                return std::make_unique<JsonConstantNode>(value.AsNumber(), DefaultPosition);

            case KgrValueType::Boolean:
                return std::make_unique<JsonConstantNode>(value.AsBoolean(), DefaultPosition);

            case KgrValueType::String: {
                std::string res;
                escape(value.AsString(), res);
                return std::make_unique<JsonConstantNode>(std::move(res), DefaultPosition);
            }

            case KgrValueType::Array: {
                std::vector<std::shared_ptr<JsonASTNode>> array;
                for (const auto &el : value.AsArray()) {
                    array.push_back(this->SerializeValue(el));
                }
                return std::make_unique<JsonArrayNode>(std::move(array), DefaultPosition);
            }

            case KgrValueType::Object: {
                std::map<std::string, std::unique_ptr<JsonASTNode>> object;
                for (const auto &kv : value.AsDictionary()) {
                    object[kv.first] = this->SerializeValue(kv.second);
                }
                return std::make_unique<JsonObjectNode>(std::move(object), DefaultPosition);
            }
        }
        return nullptr;
    }

    KgrValue KgrJsonSerializer::DeserializeValue(const JsonASTNode &node) const {
        KgrJsonDeserializeVisitor visitor;
        return node.Visit(visitor);
    }
}