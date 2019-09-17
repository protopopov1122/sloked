#include "sloked/kgr/Serialize.h"
#include "sloked/json/Parser.h"
#include <sstream>
#include <iostream>

namespace sloked {

    static const JsonSourcePosition DefaultPosition{"", 0, 0};

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

            case KgrValueType::String:
                return std::make_unique<JsonConstantNode>(value.AsString(), DefaultPosition);

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
        switch (node.GetType()) {
            case JsonASTNode::Type::Constant: {
                const JsonConstantNode &value = dynamic_cast<const JsonConstantNode &>(node);
                switch (value.GetConstantType()) {
                    case JsonConstantNode::DataType::Null:
                        return KgrValue();

                    case JsonConstantNode::DataType::Integer:
                        return KgrValue(value.AsInteger());

                    case JsonConstantNode::DataType::Number:
                        return KgrValue(value.AsNumber());

                    case JsonConstantNode::DataType::Boolean:
                        return KgrValue(value.AsBoolean());

                    case JsonConstantNode::DataType::String:
                        return KgrValue(value.AsString());
                }
            } break;
            
            case JsonASTNode::Type::Array: {
                const JsonArrayNode &value = dynamic_cast<const JsonArrayNode &>(node);
                std::vector<KgrValue> array;
                for (std::size_t i = 0; i < value.Length(); i++) {
                    array.push_back(this->DeserializeValue(value.At(i)));
                }
                return KgrValue(std::move(array));
            }
            
            case JsonASTNode::Type::Object: {
                const JsonObjectNode &value = dynamic_cast<const JsonObjectNode &>(node);
                std::map<std::string, KgrValue> object;
                for (const auto &key : value.Keys()) {
                    object[key] = this->DeserializeValue(value.Get(key));
                }
                return KgrValue(std::move(object));
            }
        }
        return KgrValue{};
    }
}