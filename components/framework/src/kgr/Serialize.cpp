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

#include "sloked/kgr/Serialize.h"

#include <limits>
#include <sstream>

#include "sloked/core/Encoding.h"
#include "sloked/core/Error.h"
#include "sloked/core/Locale.h"
#include "sloked/json/Parser.h"

namespace sloked {

    static const JsonSourcePosition DefaultPosition{"", 0, 0};

    static void unescape(std::string_view src, std::string &dst) {
        dst.reserve(src.size());
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
        dst.reserve(src.size());
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

    KgrJsonSerializer::KgrJsonSerializer(const Encoding &encoding)
        : encoding(encoding) {}

    KgrJsonSerializer::Blob KgrJsonSerializer::Serialize(
        const KgrValue &value) const {
        std::stringstream ss;
        auto node = this->SerializeValue(value);
        ss << *node;
        if (SlokedLocale::SystemEncoding() != this->encoding) {
            EncodingConverter conv(SlokedLocale::SystemEncoding(),
                                   this->encoding);
            auto raw = conv.Convert(ss.str());
            return Blob{raw.begin(), raw.end()};
        } else {
            auto raw = ss.str();
            return Blob{raw.begin(), raw.end()};
        }
    }

    KgrValue KgrJsonSerializer::Deserialize(const Blob &blob) const {
        std::stringstream ss;
        if (SlokedLocale::SystemEncoding() != this->encoding) {
            EncodingConverter conv(this->encoding,
                                   SlokedLocale::SystemEncoding());
            ss.str(conv.Convert(std::string_view{
                reinterpret_cast<const char *>(blob.data()), blob.size()}));
        } else {
            ss.str(std::string{blob.begin(), blob.end()});
        }
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

    std::unique_ptr<JsonASTNode> KgrJsonSerializer::SerializeValue(
        const KgrValue &value) const {
        switch (value.GetType()) {
            case KgrValueType::Null:
                return std::make_unique<JsonConstantNode>(DefaultPosition);

            case KgrValueType::Integer:
                return std::make_unique<JsonConstantNode>(value.AsInt(),
                                                          DefaultPosition);

            case KgrValueType::Number:
                return std::make_unique<JsonConstantNode>(value.AsNumber(),
                                                          DefaultPosition);

            case KgrValueType::Boolean:
                return std::make_unique<JsonConstantNode>(value.AsBoolean(),
                                                          DefaultPosition);

            case KgrValueType::String: {
                std::string res;
                escape(value.AsString(), res);
                return std::make_unique<JsonConstantNode>(std::move(res),
                                                          DefaultPosition);
            }

            case KgrValueType::Array: {
                std::vector<std::shared_ptr<JsonASTNode>> array;
                for (const auto &el : value.AsArray()) {
                    array.push_back(this->SerializeValue(el));
                }
                return std::make_unique<JsonArrayNode>(std::move(array),
                                                       DefaultPosition);
            }

            case KgrValueType::Object: {
                std::map<std::string, std::unique_ptr<JsonASTNode>> object;
                for (const auto &kv : value.AsDictionary()) {
                    object[kv.first] = this->SerializeValue(kv.second);
                }
                return std::make_unique<JsonObjectNode>(std::move(object),
                                                        DefaultPosition);
            }
        }
        return nullptr;
    }

    KgrValue KgrJsonSerializer::DeserializeValue(
        const JsonASTNode &node) const {
        KgrJsonDeserializeVisitor visitor;
        return node.Visit(visitor);
    }

    enum class KgrBinarySerializer::Tag {
        Null = 1,
        Integer8,
        Integer16,
        Integer32,
        Integer64,
        Float,
        BooleanTrue,
        BooleanFalse,
        String,
        Array,
        Object
    };

    class KgrBinarySerializer::ByteIter {
     public:
        ByteIter(std::vector<SlokedBase64::Byte>::const_iterator begin,
                 std::vector<SlokedBase64::Byte>::const_iterator end)
            : current(std::move(begin)), end(std::move(end)) {}

        uint8_t Next() {
            if (current == end) {
                throw SlokedError("BinarySerializer: Unexpected end of binary");
            }
            return *this->current++;
        }

     private:
        std::vector<SlokedBase64::Byte>::const_iterator current;
        std::vector<SlokedBase64::Byte>::const_iterator end;
    };

    KgrBinarySerializer::KgrBinarySerializer(const Encoding &encoding)
        : encoding(encoding) {}

    KgrSerializer::Blob KgrBinarySerializer::Serialize(
        const KgrValue &value) const {
        KgrSerializer::Blob blob{};
        this->SerializeValue(blob, value);
        return blob;
    }

    KgrValue KgrBinarySerializer::Deserialize(
        const KgrSerializer::Blob &blob) const {
        ByteIter iter(blob.begin(), blob.end());
        return this->DeserializeValue(iter);
    }

    KgrValue KgrBinarySerializer::Deserialize(std::istream &is) const {
        std::vector<uint8_t> blob(std::istreambuf_iterator<char>(is), {});
        ByteIter iter(blob.begin(), blob.end());
        return this->DeserializeValue(iter);
    }

    template <typename T, typename I, std::size_t Bytes = sizeof(T)>
    void SerializeScalar([[maybe_unused]] T value,
                         [[maybe_unused]] I inserter) {
        static_assert(Bytes <= sizeof(uint64_t));
        if constexpr (Bytes > 0) {
            union {
                uint64_t raw;
                T value;
            } uvalue;
            uvalue.raw = 0;
            uvalue.value = value;
            uint8_t byte = uvalue.raw & 0xff;
            inserter = byte;
            (SerializeScalar<uint64_t, I, (Bytes - 1)>) (uvalue.raw >> 8,
                                                         ++inserter);
        }
    }

    template <typename T, typename I>
    void SerializeInt(T value, I inserter) {
        if (value >= std::numeric_limits<int8_t>::min() &&
            value <= std::numeric_limits<int8_t>::max()) {
            inserter = static_cast<int8_t>(KgrBinarySerializer::Tag::Integer8);
            SerializeScalar(static_cast<int8_t>(value), ++inserter);
        } else if (value >= std::numeric_limits<int16_t>::min() &&
                   value <= std::numeric_limits<int16_t>::max()) {
            inserter = static_cast<int8_t>(KgrBinarySerializer::Tag::Integer16);
            SerializeScalar(static_cast<int16_t>(value), ++inserter);
        } else if (value >= std::numeric_limits<int32_t>::min() &&
                   value <= std::numeric_limits<int32_t>::max()) {
            inserter = static_cast<int8_t>(KgrBinarySerializer::Tag::Integer32);
            SerializeScalar(static_cast<int32_t>(value), ++inserter);
        } else {
            inserter = static_cast<int8_t>(KgrBinarySerializer::Tag::Integer64);
            SerializeScalar(value, ++inserter);
        }
    }

    void KgrBinarySerializer::SerializeValue(KgrSerializer::Blob &output,
                                             const KgrValue &value) const {
        switch (value.GetType()) {
            case KgrValueType::Null:
                output.push_back(static_cast<int8_t>(Tag::Null));
                break;

            case KgrValueType::Integer:
                SerializeInt(value.AsInt(), std::back_inserter(output));
                break;

            case KgrValueType::Number:
                output.push_back(static_cast<int8_t>(Tag::Float));
                SerializeScalar(value.AsNumber(), std::back_inserter(output));
                break;

            case KgrValueType::Boolean:
                output.push_back(static_cast<int8_t>(
                    value.AsBoolean() ? Tag::BooleanTrue : Tag::BooleanFalse));
                break;

            case KgrValueType::String:
                output.push_back(static_cast<int8_t>(Tag::String));
                if (SlokedLocale::SystemEncoding() != this->encoding) {
                    EncodingConverter conv(SlokedLocale::SystemEncoding(),
                                           this->encoding);
                    std::string decoded = conv.Convert(value.AsString());
                    if (decoded.size() > std::numeric_limits<uint32_t>::max()) {
                        throw SlokedError(
                            "BinarySerializer: String length exceeds maximum");
                    }
                    SerializeScalar(static_cast<uint32_t>(decoded.size()),
                                    std::back_inserter(output));
                    output.insert(output.end(), decoded.begin(), decoded.end());
                } else {
                    if (value.AsString().size() >
                        std::numeric_limits<uint32_t>::max()) {
                        throw SlokedError(
                            "BinarySerializer: String length exceeds maximum");
                    }
                    SerializeScalar(
                        static_cast<uint32_t>(value.AsString().size()),
                        std::back_inserter(output));
                    output.insert(output.end(), value.AsString().begin(),
                                  value.AsString().end());
                }
                break;

            case KgrValueType::Array: {
                const auto &array = value.AsArray();
                if (array.Size() > std::numeric_limits<uint32_t>::max()) {
                    throw SlokedError(
                        "BinarySerializer: Array length exceeds maximum");
                }
                output.push_back(static_cast<int8_t>(Tag::Array));
                SerializeScalar(static_cast<uint32_t>(array.Size()),
                                std::back_inserter(output));
                for (const auto &el : array) {
                    this->SerializeValue(output, el);
                }
            } break;

            case KgrValueType::Object: {
                const auto &object = value.AsDictionary();
                if (object.Size() > std::numeric_limits<uint32_t>::max()) {
                    throw SlokedError(
                        "BinarySerializer: Object field count exceeds maximum");
                }
                output.push_back(static_cast<int8_t>(Tag::Object));
                SerializeScalar(static_cast<uint32_t>(object.Size()),
                                std::back_inserter(output));
                for (const auto &el : object) {
                    if (SlokedLocale::SystemEncoding() != this->encoding) {
                        EncodingConverter conv(SlokedLocale::SystemEncoding(),
                                               this->encoding);
                        std::string decoded = conv.Convert(el.first);
                        SerializeScalar(static_cast<uint32_t>(decoded.size()),
                                        std::back_inserter(output));
                        output.insert(output.end(), decoded.begin(),
                                      decoded.end());
                    } else {
                        SerializeScalar(static_cast<uint32_t>(el.first.size()),
                                        std::back_inserter(output));
                        output.insert(output.end(), el.first.begin(),
                                      el.first.end());
                    }
                    this->SerializeValue(output, el.second);
                }
            } break;
        }
    }

    template <typename T, std::size_t Bytes = sizeof(T)>
    T DeserializeScalar(KgrBinarySerializer::ByteIter &iter,
                        uint64_t value = 0) {
        if constexpr (Bytes > 0) {
            return DeserializeScalar<T, Bytes - 1>(
                iter, value | (static_cast<uint64_t>(iter.Next())
                               << ((sizeof(T) - Bytes) * 8)));
        } else {
            union {
                uint64_t raw;
                T value;
            } uvalue;
            uvalue.raw = value;
            return uvalue.value;
        }
    }

    KgrValue KgrBinarySerializer::DeserializeValue(ByteIter &iter) const {
        auto tag = static_cast<Tag>(iter.Next());
        switch (tag) {
            case Tag::Null:
                return {};

            case Tag::Integer8:
                return static_cast<int64_t>(DeserializeScalar<int8_t>(iter));

            case Tag::Integer16:
                return static_cast<int64_t>(DeserializeScalar<int16_t>(iter));

            case Tag::Integer32:
                return static_cast<int64_t>(DeserializeScalar<int32_t>(iter));

            case Tag::Integer64:
                return DeserializeScalar<int64_t>(iter);

            case Tag::Float:
                return DeserializeScalar<double>(iter);

            case Tag::BooleanTrue:
                return true;

            case Tag::BooleanFalse:
                return false;

            case Tag::String: {
                std::size_t length = DeserializeScalar<uint32_t>(iter);
                std::string value;
                value.reserve(length);
                while (length--) {
                    value.push_back(iter.Next());
                }
                if (SlokedLocale::SystemEncoding() != this->encoding) {
                    EncodingConverter conv(this->encoding,
                                           SlokedLocale::SystemEncoding());
                    return conv.Convert(value);
                } else {
                    return value;
                }
            }

            case Tag::Array: {
                std::size_t length = DeserializeScalar<uint32_t>(iter);
                std::vector<KgrValue> raw;
                raw.reserve(length);
                while (length--) {
                    raw.emplace_back(this->DeserializeValue(iter));
                }
                return KgrArray{std::move(raw)};
            }

            case Tag::Object: {
                KgrDictionary object;
                std::size_t length = DeserializeScalar<uint32_t>(iter);
                std::string key;
                while (length--) {
                    std::size_t keyLength = DeserializeScalar<uint32_t>(iter);
                    key.clear();
                    key.reserve(keyLength);
                    while (keyLength--) {
                        key.push_back(iter.Next());
                    }
                    if (SlokedLocale::SystemEncoding() != this->encoding) {
                        EncodingConverter conv(this->encoding,
                                               SlokedLocale::SystemEncoding());
                        key = conv.Convert(key);
                    }
                    object.Put(key, this->DeserializeValue(iter));
                }
                return object;
            }
        }
        return {};
    }
}  // namespace sloked