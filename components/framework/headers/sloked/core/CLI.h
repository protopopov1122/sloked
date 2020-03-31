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

#ifndef SLOKED_CORE_CLI_H_
#define SLOKED_CORE_CLI_H_

#include <cinttypes>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "sloked/core/Error.h"
#include "sloked/core/String.h"
#include "sloked/kgr/Value.h"
#include "sloked/namespace/Path.h"

namespace sloked {

    class SlokedCLIArgumentIterator {
     public:
        SlokedCLIArgumentIterator(std::size_t, const char **);
        bool HasNext() const;
        std::string_view Next();

     private:
        std::size_t argc;
        const char **argv;
    };

    class SlokedCLIValue {
        template <typename T, typename E = void>
        struct Conversion;

        template <typename T>
        struct Conversion<T, std::enable_if_t<!std::is_same_v<T, bool> &&
                                              std::is_integral_v<T>>> {
            static T Apply(const KgrValue &value) {
                if (value.Is(KgrValueType::Integer)) {
                    return static_cast<T>(value.AsInt());
                } else {
                    throw SlokedError(
                        "CLIValue: Error converting value to integer");
                }
            }
        };

        template <typename T>
        struct Conversion<T, std::enable_if_t<std::is_floating_point_v<T>>> {
            static T Apply(const KgrValue &value) {
                if (value.Is(KgrValueType::Number)) {
                    return static_cast<T>(value.AsNumber());
                } else {
                    throw SlokedError(
                        "CLIValue: Error converting value to float");
                }
            }
        };

        template <typename T>
        struct Conversion<T, std::enable_if_t<std::is_same_v<T, bool>>> {
            static T Apply(const KgrValue &value) {
                if (value.Is(KgrValueType::Boolean)) {
                    return value.AsBoolean();
                } else {
                    throw SlokedError(
                        "CLIValue: Error converting value to boolean");
                }
            }
        };

        template <typename T>
        struct Conversion<T, std::enable_if_t<std::is_same_v<T, std::string>>> {
            static const T &Apply(const KgrValue &value) {
                if (value.Is(KgrValueType::String)) {
                    return value.AsString();
                } else {
                    throw SlokedError(
                        "CLIValue: Error converting value to string");
                }
            }
        };

     public:
        enum class Type { Integer, Float, Boolean, String };

        SlokedCLIValue(int64_t);
        SlokedCLIValue(double);
        SlokedCLIValue(bool);
        SlokedCLIValue(std::string_view);
        SlokedCLIValue(const std::string &);
        SlokedCLIValue(const char *);

        Type GetType() const;
        const KgrValue &GetValue() const;

        template <typename T>
        auto As() const {
            return Conversion<T>::Apply(this->value);
        }

        template <typename T>
        static SlokedCLIValue Make(
            std::enable_if_t<!std::is_same_v<T, bool> && std::is_integral_v<T>,
                             T>
                value) {
            return SlokedCLIValue(static_cast<int64_t>(value));
        }

        template <typename T>
        static SlokedCLIValue Make(
            std::enable_if_t<std::is_floating_point_v<T>, T> value) {
            return SlokedCLIValue(static_cast<double>(value));
        }

        template <typename T>
        static SlokedCLIValue Make(
            std::enable_if_t<std::is_same_v<T, bool>, T> value) {
            return SlokedCLIValue(value);
        }

        template <typename T>
        static SlokedCLIValue Make(
            std::enable_if_t<std::is_same_v<T, std::string> ||
                                 std::is_same_v<T, std::string_view> ||
                                 std::is_same_v<std::remove_cv_t<T>, char *>,
                             T>
                value) {
            return SlokedCLIValue(std::move(value));
        }

     private:
        KgrValue value;
    };

    class SlokedCLIOption {
     public:
        SlokedCLIOption(SlokedCLIValue::Type);
        SlokedCLIOption(SlokedCLIValue);

        SlokedCLIValue::Type Type() const;
        bool Empty() const;
        void Map(const SlokedPath &);
        bool Export(KgrValue &);
        bool IsMandatory() const;
        SlokedCLIOption &Mandatory(bool = true);

        template <typename T>
        auto As() const {
            if (this->value.has_value()) {
                return this->value.value().As<T>();
            } else {
                throw SlokedError("CLIOption: Undefined option value");
            }
        }

        template <typename T>
        auto As(T &&defaultValue) const {
            if (this->value.has_value()) {
                return this->value.value().As<T>();
            } else {
                return defaultValue;
            }
        }

        template <typename T>
        SlokedCLIOption &Set(T &&nValue) {
            auto newValue = SlokedCLIValue::Make<
                std::remove_cv_t<std::remove_reference_t<T>>>(
                std::forward<T>(nValue));
            if (newValue.GetType() == this->type) {
                this->value = newValue;
                return *this;
            } else {
                throw SlokedError("CLIOption: Type conversion error");
            }
        }

        template <typename T>
        SlokedCLIOption &Fallback(T &&value) {
            if (!this->value.has_value()) {
                this->Set(std::forward<T>(value));
            }
            return *this;
        }

     private:
        SlokedCLIValue::Type type;
        std::optional<SlokedCLIValue> value;
        std::optional<SlokedPath> path;
        bool mandatory;
    };

    class SlokedCLI {
        template <typename T, typename E = void>
        struct TypeSwitcher;

        template <typename T>
        struct TypeSwitcher<T, std::enable_if_t<!std::is_same_v<T, bool> &&
                                                std::is_integral_v<T>>> {
            static constexpr auto Type = SlokedCLIValue::Type::Integer;
        };

        template <typename T>
        struct TypeSwitcher<T, std::enable_if_t<std::is_floating_point_v<T>>> {
            static constexpr auto Type = SlokedCLIValue::Type::Float;
        };

        template <typename T>
        struct TypeSwitcher<T, std::enable_if_t<std::is_same_v<T, bool>>> {
            static constexpr auto Type = SlokedCLIValue::Type::Boolean;
        };

        template <typename T>
        struct TypeSwitcher<T,
                            std::enable_if_t<std::is_same_v<T, std::string>>> {
            static constexpr auto Type = SlokedCLIValue::Type::String;
        };

     public:
        using Iterator = std::vector<std::string_view>::const_iterator;
        bool Has(const std::string &) const;
        bool Has(char) const;
        std::size_t ArgCount() const;
        const SlokedCLIOption &operator[](const std::string &) const;
        const SlokedCLIOption &operator[](char) const;
        std::string_view Argument(std::size_t) const;
        Iterator begin() const;
        Iterator end() const;
        void Parse(int, const char **, bool = false);
        KgrValue Export() const;
        void Initialize(const KgrValue &);

        template <typename T>
        SlokedCLIOption &Define(const std::string &keys, T &&value,
                                const std::string &description = "") {
            std::shared_ptr<SlokedCLIOption> option;
            if constexpr (std::is_same_v<T, SlokedCLIValue> ||
                          std::is_same_v<T, SlokedCLIValue::Type> ||
                          std::is_same_v<std::remove_reference_t<T>,
                                         SlokedCLIOption>) {
                option =
                    std::make_shared<SlokedCLIOption>(std::forward<T>(value));
            } else {
                option = std::make_shared<SlokedCLIOption>(
                    this->Option(std::forward<T>(value)));
            }
            this->DefineImpl(keys, option);
            this->descriptions.push_back(
                std::make_pair(OptionDescription{keys, description}, option));
            return *option;
        }

        template <typename T>
        void Fallback(const std::string &keys, T &&value) {
            auto options = this->FindKeys(keys);
            for (auto &option : options) {
                option->Fallback<T>(std::forward<T>(value));
            }
        }

        template <typename T>
        auto Option() const {
            return TypeSwitcher<T>::Type;
        }

        template <typename T>
        auto Option(T &&value) const {
            if constexpr (std::is_same_v<T, bool>) {
                return SlokedCLIValue(std::forward<T>(value));
            } else if constexpr (std::is_integral_v<T>) {
                return SlokedCLIValue(static_cast<int64_t>(value));
            } else if constexpr (std::is_floating_point_v<T>) {
                return SlokedCLIValue(static_cast<double>(value));
            } else {
                return SlokedCLIValue(std::forward<T>(value));
            }
        }

     private:
        void ParseOption(std::string_view, SlokedCLIArgumentIterator &, bool);
        void ParseShortOption(std::string_view, SlokedCLIArgumentIterator &,
                              bool);
        void DefineImpl(std::string_view, std::shared_ptr<SlokedCLIOption>);
        std::vector<std::shared_ptr<SlokedCLIOption>> FindKeys(
            std::string_view);

        struct OptionDescription {
            std::string keys;
            std::string description;
        };

        std::map<std::string, std::shared_ptr<SlokedCLIOption>> options;
        std::map<char, std::shared_ptr<SlokedCLIOption>> shortOptions;
        std::vector<std::string_view> arguments;
        std::vector<
            std::pair<OptionDescription, std::shared_ptr<SlokedCLIOption>>>
            descriptions;
    };
}  // namespace sloked

#endif