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

#include "sloked/kgr/Value.h"
#include "sloked/core/Error.h"

namespace sloked {

    KgrArray::KgrArray(const std::vector<KgrValue> &value)
        : content(value) {}
    
    KgrArray::KgrArray(std::vector<KgrValue> &&value)
        : content(std::forward<std::vector<KgrValue>>(value)) {}
    
    KgrArray::KgrArray(std::initializer_list<KgrValue> content)
        : content(std::move(content)) {}

    std::size_t KgrArray::Size() const {
        return this->content.size();
    }

    bool KgrArray::Empty() const {
        return this->content.empty();
    }

    const KgrValue &KgrArray::At(std::size_t idx) const {
        if (idx < this->content.size()) {
            return this->content.at(idx);
        } else {
            throw SlokedError("KgrArray: Out of bounds error");
        }
    }

    const KgrValue &KgrArray::operator[](std::size_t idx) const {
        return this->At(idx);
    }

    KgrValue &KgrArray::operator[](std::size_t idx) {
        if (idx < this->content.size()) {
            return this->content[idx];
        } else {
            throw SlokedError("KgrArray: Out of bounds error");
        }
    }

    KgrArray::ConstIterator KgrArray::begin() const {
        return this->content.begin();
    }

    KgrArray::ConstIterator KgrArray::end() const {
        return this->content.end();
    }

    KgrArray::ConstReverseIterator KgrArray::rbegin() const {
        return this->content.rbegin();
    }

    KgrArray::ConstReverseIterator KgrArray::rend() const {
        return this->content.rend();
    }
    
    KgrArray &KgrArray::Append(const KgrValue &value) {
        this->content.push_back(value);
        return *this;
    }

    KgrArray &KgrArray::Append(KgrValue &&value) {
        this->content.push_back(std::forward<KgrValue>(value));
        return *this;
    }

    KgrArray &KgrArray::Replace(std::size_t idx, const KgrValue &value) {
        if (idx < this->content.size()) {
            this->content[idx] = value;
            return *this;
        } else {
            throw SlokedError("KgrArray: Out of bounds error");
        }
    }

    KgrArray &KgrArray::Replace(std::size_t idx, KgrValue &&value) {
        if (idx < this->content.size()) {
            this->content[idx] = value;
            return *this;
        } else {
            throw SlokedError("KgrArray: Out of bounds error");
        }
    }

    KgrArray &KgrArray::Insert(std::size_t idx, const KgrValue &value) {
        if (idx <= this->content.size()) {
            this->content.insert(this->content.begin() + idx, value);
            return *this;
        } else {
            throw SlokedError("KgrArray: Out of bounds error");
        }
    }

    KgrArray &KgrArray::Insert(std::size_t idx, KgrValue &&value) {
        if (idx <= this->content.size()) {
            this->content.insert(this->content.begin() + idx, std::forward<KgrValue>(value));
            return *this;
        } else {
            throw SlokedError("KgrArray: Out of bounds error");
        }
    }
    
    KgrArray &KgrArray::Remove(std::size_t idx) {
        if (idx < this->content.size()) {
            this->content.erase(this->content.begin() + idx);
            return *this;
        } else {
            throw SlokedError("KgrArray: Out of bounds error");
        }
    }

    KgrDictionary::KgrDictionary(const std::map<std::string, KgrValue> &content)
        : content(content) {}

    KgrDictionary::KgrDictionary(std::map<std::string, KgrValue> &&content)
        : content(content) {}

    KgrDictionary::KgrDictionary(std::initializer_list<std::pair<const std::string, KgrValue>> content)
        : content(std::move(content)) {}
    
    std::size_t KgrDictionary::Size() const {
        return this->content.size();
    }

    bool KgrDictionary::Empty() const {
        return this->content.empty();
    }

    bool KgrDictionary::Has(const std::string &key) const {
        return this->content.count(key) != 0;
    }

    const KgrValue &KgrDictionary::Get(const std::string &key) const {
        if (this->Has(key)) {
            return this->content.at(key);
        } else {
            throw SlokedError("KgrDictionary: Unknown key \'" + key + "\'");
        }
    }

    bool KgrDictionary::Has(const char *key) const {
        return this->content.count(key) != 0;
    }

    const KgrValue &KgrDictionary::Get(const char *key) const {
        if (this->Has(key)) {
            return this->content.at(key);
        } else {
            throw SlokedError("KgrDictionary: Unknown key \'" + std::string{key} + "\'");
        }
    }

    const KgrValue &KgrDictionary::operator[](const std::string &key) const {
        if (this->Has(key)) {
            return this->content.at(key);
        } else {
            throw SlokedError("KgrDictionary: Unknown key \'" + key + "\'");
        }
    }

    KgrValue &KgrDictionary::operator[](const char *key) {
        if (this->Has(key)) {
            return this->content[key];
        } else {
            throw SlokedError("KgrDictionary: Unknown key \'" + std::string{key} + "\'");
        }
    }

    const KgrValue &KgrDictionary::operator[](const char *key) const {
        if (this->Has(key)) {
            return this->content.at(key);
        } else {
            throw SlokedError("KgrDictionary: Unknown key \'" + std::string{key} + "\'");
        }
    }

    KgrValue &KgrDictionary::operator[](const std::string &key) {
        if (this->Has(key)) {
            return this->content[key];
        } else {
            throw SlokedError("KgrDictionary: Unknown key \'" + key + "\'");
        }
    }

    KgrDictionary::ConstIterator KgrDictionary::begin() const {
        return this->content.begin();
    }

    KgrDictionary::ConstIterator KgrDictionary::end() const {
        return this->content.end();
    }

    KgrDictionary::ConstReverseIterator KgrDictionary::rbegin() const {
        return this->content.rbegin();
    }

    KgrDictionary::ConstReverseIterator KgrDictionary::rend() const {
        return this->content.rend();
    }

    KgrDictionary &KgrDictionary::Put(const std::string &key, const KgrValue &value) {
        this->content[key] = value;
        return *this;
    }

    KgrDictionary &KgrDictionary::Put(const std::string &key, KgrValue &&value) {
        this->content.emplace(key, std::forward<KgrValue>(value));
        return *this;
    }

    KgrDictionary &KgrDictionary::Remove(const std::string &key) {
        if (this->Has(key)) {
            this->content.erase(key);
            return *this;
        } else {
            throw SlokedError("KgrDictionary: Unknown key \'" + key + "\'");
        }
    }

    KgrValue::KgrValue()
        : type(KgrValueType::Null) {}
        
    KgrValue::KgrValue(int64_t value)
        : type(KgrValueType::Integer), value(value) {}
        
    KgrValue::KgrValue(int value)
        : type(KgrValueType::Integer), value(int64_t{value}) {}

    KgrValue::KgrValue(double value)
        : type(KgrValueType::Number), value(value) {}
    
    KgrValue::KgrValue(bool value)
        : type(KgrValueType::Boolean), value(value) {}
    
    KgrValue::KgrValue(std::string_view value)
        : type(KgrValueType::String), value(std::string{value}) {}

    KgrValue::KgrValue(const std::string &value)
        : type(KgrValueType::String), value(value) {}
    
    KgrValue::KgrValue(const char *value)
        : type(KgrValueType::String), value(std::string{value}) {}

    KgrValue::KgrValue(const KgrArray &value)
        : type(KgrValueType::Array), value(value) {}
    
    KgrValue::KgrValue(KgrArray &&value)
        : type(KgrValueType::Array), value(std::forward<KgrArray>(value)) {}

    KgrValue::KgrValue(const KgrDictionary &value)
        : type(KgrValueType::Object), value(value) {}

    KgrValue::KgrValue(KgrDictionary &&value)
        : type(KgrValueType::Object), value(std::forward<KgrDictionary>(value)) {}

    KgrValueType KgrValue::GetType() const {
        return this->type;
    }

    bool KgrValue::Is(KgrValueType type) const {
        return this->type == type;
    }

    int64_t KgrValue::AsInt() const {
        if (this->type == KgrValueType::Integer) {
            return std::get<0>(this->value);
        } else {
            throw SlokedError("KgrValue: Invalid conversion");
        }
    }

    double KgrValue::AsNumber() const {
        if (this->type == KgrValueType::Number) {
            return std::get<1>(this->value);
        } else {
            throw SlokedError("KgrValue: Invalid conversion");
        }
    }

    bool KgrValue::AsBoolean() const {
        if (this->type == KgrValueType::Boolean) {
            return std::get<2>(this->value);
        } else {
            throw SlokedError("KgrValue: Invalid conversion");
        }
    }
    
    const std::string &KgrValue::AsString() const {
        if (this->type == KgrValueType::String) {
            return std::get<3>(this->value);
        } else {
            throw SlokedError("KgrValue: Invalid conversion");
        }
    }

    const KgrArray &KgrValue::AsArray() const {
        if (this->type == KgrValueType::Array) {
            return std::get<4>(this->value);
        } else {
            throw SlokedError("KgrValue: Invalid conversion");
        }
    }

    KgrArray &KgrValue::AsArray() {
        if (this->type == KgrValueType::Array) {
            return std::get<4>(this->value);
        } else {
            throw SlokedError("KgrValue: Invalid conversion");
        }
    }

    const KgrDictionary &KgrValue::AsDictionary() const {
        if (this->type == KgrValueType::Object) {
            return std::get<5>(this->value);
        } else {
            throw SlokedError("KgrValue: Invalid conversion");
        }
    }
    
    KgrDictionary &KgrValue::AsDictionary() {
        if (this->type == KgrValueType::Object) {
            return std::get<5>(this->value);
        } else {
            throw SlokedError("KgrValue: Invalid conversion");
        }
    }
}