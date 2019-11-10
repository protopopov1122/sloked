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

#ifndef SLOKED_CLI_OPTIONS_H_
#define SLOKED_CLI_OPTIONS_H_

#include "sloked/core/Error.h"
#include <variant>
#include <string>
#include <cinttypes>
#include <type_traits>
#include <map>
#include <vector>
#include <optional>
#include <utility>

namespace sloked {

    class SlokedCLIArguments {
     public:
        SlokedCLIArguments(std::size_t, const char **);
        bool HasNext() const;
        std::string_view Next();
        std::string_view Current();

     private:
        std::size_t argc;
        const char **argv;
    };

    class SlokedCLIValue {
     public:
        enum class Type {
            Integer, Float, Boolean, String
        };

        SlokedCLIValue(int64_t);
        SlokedCLIValue(double);
        SlokedCLIValue(bool);
        SlokedCLIValue(std::string_view);
        SlokedCLIValue(const std::string &);

        Type GetType() const;
        bool Is(Type) const;
        int64_t AsInt() const;
        double AsFloat() const;
        bool AsBoolean() const;
        const std::string &AsString() const;

     private:
        std::variant<int64_t, double, bool, std::string> value;
    };

    class SlokedCLIOption {
     public:
        SlokedCLIOption(SlokedCLIValue::Type);
        SlokedCLIOption(SlokedCLIValue &&);

        SlokedCLIValue::Type GetType() const;
        bool HasValue() const;
        int64_t AsInt() const;
        double AsFloat() const;
        bool AsBoolean() const;
        const std::string &AsString() const;

        void SetValue(int64_t);
        void SetValue(double);
        void SetValue(bool);
        void SetValue(std::string_view);

     private:
        SlokedCLIValue::Type type;
        std::optional<SlokedCLIValue> value;
    };

    class SlokedCLI {
     public:
        bool Has(const std::string &) const;
        bool Has(char) const;
        std::size_t ArgCount() const;
        const SlokedCLIOption &operator[](const std::string &) const;
        const SlokedCLIOption &operator[](char) const;
        std::string_view At(std::size_t) const;
        void Define(const std::string &, SlokedCLIOption &&);
        void Define(char, const std::string &, SlokedCLIOption &&);
        void Parse(SlokedCLIArguments &);
    
     private:
        void ParseOption(std::string_view, SlokedCLIArguments &);
        void ParseShortOption(std::string_view, SlokedCLIArguments &);

        std::map<std::string, SlokedCLIOption> options;
        std::map<char, SlokedCLIOption *> shortOptions;
        std::vector<std::string_view> arguments;
    };
}

#endif