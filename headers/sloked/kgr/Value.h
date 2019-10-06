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

#ifndef SLOKED_KGR_OBJECT_H_
#define SLOKED_KGR_OBJECT_H_

#include "sloked/Base.h"
#include <variant>
#include <string>
#include <cinttypes>
#include <vector>
#include <map>

namespace sloked {

    enum class KgrValueType {
        Integer,
        Number,
        Boolean,
        String,
        Null,
        Array,
        Object
    };

    class KgrValue;

    class KgrArray {
     public:
        using ConstIterator = std::vector<KgrValue>::const_iterator;
        using ConstReverseIterator = std::vector<KgrValue>::const_reverse_iterator;

        KgrArray() = default;
        KgrArray(const std::vector<KgrValue> &);
        KgrArray(std::vector<KgrValue> &&);
        KgrArray(std::initializer_list<KgrValue>);
        KgrArray(const KgrArray &) = default;
        KgrArray(KgrArray &&) = default;

        KgrArray &operator=(const KgrArray &) = default;
        KgrArray &operator=(KgrArray &&) = default;

        std::size_t Size() const;
        bool Empty() const;
        const KgrValue &At(std::size_t) const;
        const KgrValue &operator[](std::size_t) const;
        KgrValue &operator[](std::size_t);

        ConstIterator begin() const;
        ConstIterator end() const;
        ConstReverseIterator rbegin() const;
        ConstReverseIterator rend() const;
        
        KgrArray &Append(const KgrValue &);
        KgrArray &Append(KgrValue &&);
        KgrArray &Replace(std::size_t, const KgrValue &);
        KgrArray &Replace(std::size_t, KgrValue &&);
        KgrArray &Insert(std::size_t, const KgrValue &);
        KgrArray &Insert(std::size_t, KgrValue &&);
        KgrArray &Remove(std::size_t);

     private:
        std::vector<KgrValue> content;
    };

    class KgrDictionary {
     public:
        using ConstIterator = std::map<std::string, KgrValue>::const_iterator;
        using ConstReverseIterator = std::map<std::string, KgrValue>::const_reverse_iterator;

        KgrDictionary() = default;
        KgrDictionary(const std::map<std::string, KgrValue> &);
        KgrDictionary(std::map<std::string, KgrValue> &&);
        KgrDictionary(std::initializer_list<std::pair<const std::string, KgrValue>>);
        KgrDictionary(const KgrDictionary &) = default;
        KgrDictionary(KgrDictionary &&) = default;

        KgrDictionary &operator=(const KgrDictionary &) = default;
        KgrDictionary &operator=(KgrDictionary &&) = default;

        std::size_t Size() const;
        bool Empty() const;
        bool Has(const std::string &) const;
        const KgrValue &Get(const std::string &) const;
        const KgrValue &operator[](const std::string &) const;
        KgrValue &operator[](const std::string &);

        ConstIterator begin() const;
        ConstIterator end() const;
        ConstReverseIterator rbegin() const;
        ConstReverseIterator rend() const;

        KgrDictionary &Put(const std::string &, const KgrValue &);
        KgrDictionary &Remove(const std::string &);

     private:
        std::map<std::string, KgrValue> content;
    };

    class KgrValue {
     public:
        KgrValue();
        KgrValue(int64_t);
        KgrValue(int);
        KgrValue(double);
        KgrValue(bool);
        KgrValue(std::string_view);
        KgrValue(const std::string &);
        KgrValue(const char *);
        KgrValue(const KgrArray &);
        KgrValue(KgrArray &&);
        KgrValue(const KgrDictionary &);
        KgrValue(KgrDictionary &&);
        KgrValue(const KgrValue &) = default;
        KgrValue(KgrValue &&) = default;

        KgrValue &operator=(const KgrValue &) = default;
        KgrValue &operator=(KgrValue &&) = default;

        KgrValueType GetType() const;
        bool Is(KgrValueType) const;

        int64_t AsInt() const;
        double AsNumber() const;
        bool AsBoolean() const;
        const std::string &AsString() const;
        const KgrArray &AsArray() const;
        KgrArray &AsArray();
        const KgrDictionary &AsDictionary() const;
        KgrDictionary &AsDictionary();

     private:
        KgrValueType type;
        std::variant<int64_t, double, bool, std::string, KgrArray, KgrDictionary> value;
    };
}

#endif