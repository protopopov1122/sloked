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

#ifndef SLOKED_CORE_CLI_H_
#define SLOKED_CORE_CLI_H_

#include "sloked/core/Error.h"
#include "sloked/core/String.h"
#include <variant>
#include <string>
#include <cinttypes>
#include <type_traits>
#include <map>
#include <vector>
#include <optional>
#include <utility>
#include <memory>

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
    	using ValueType = std::variant<int64_t, double, bool, std::string>;

		template <typename T, typename E = void>
		struct Conversion;

		template <typename T>
		struct Conversion<T, std::enable_if_t<std::is_integral_v<T>>> {
			static T Apply(const ValueType &value) {
				if (value.index() == 0) {
					return static_cast<T>(std::get<0>(value));
				} else {
					throw SlokedError("CLIValue: Error converting " + std::string(SlokedCLIValue::TypeToName(static_cast<Type>(value.index()))) + " to integer");
				}
			}
		};

		template <typename T>
		struct Conversion<T, std::enable_if_t<std::is_floating_point_v<T>>> {
			static T Apply(const ValueType &value) {
				if (value.index() == 1) {
					return static_cast<T>(std::get<1>(value));
				} else {
					throw SlokedError("CLIValue: Error converting " + std::string(SlokedCLIValue::TypeToName(static_cast<Type>(value.index()))) + " to float");
				}
			}
		};

		template <typename T>
		struct Conversion<T, std::enable_if_t<std::is_same_v<T, bool>>> {
			static T Apply(const ValueType &value) {
				if (value.index() == 2) {
					return std::get<2>(value);
				} else {
					throw SlokedError("CLIValue: Error converting " + std::string(SlokedCLIValue::TypeToName(static_cast<Type>(value.index()))) + " to boolean");
				}
			}
		};

		template <typename T>
		struct Conversion<T, std::enable_if_t<std::is_same_v<T, std::string>>> {
			static const T &Apply(const ValueType &value) {
				if (value.index() == 3) {
					return std::get<3>(value);
				} else {
					throw SlokedError("CLIValue: Error converting " + std::string(SlokedCLIValue::TypeToName(static_cast<Type>(value.index()))) + " to string");
				}
			}
		};

     public:
        enum class Type {
            Integer, Float, Boolean, String
        };

        SlokedCLIValue(int64_t);
        SlokedCLIValue(double);
        SlokedCLIValue(bool);
        SlokedCLIValue(std::string_view);
        SlokedCLIValue(const std::string &);
        SlokedCLIValue(const char *);

        Type GetType() const;

		template <typename T>
		auto As() const {
			return Conversion<T>::Apply(this->value);
		}

		static constexpr const char *TypeToName(Type type) {
			switch (type) {
				case Type::Integer:
					return "integer";

				case Type::Float:
					return "float";

				case Type::Boolean:
					return "boolean";

				case Type::String:
					return "string";
			}
			return "";
		}

     private:
        std::variant<int64_t, double, bool, std::string> value;
    };

    class SlokedCLIOption {
     public:
        SlokedCLIOption(SlokedCLIValue::Type);
        SlokedCLIOption(SlokedCLIValue &&);

        SlokedCLIValue::Type GetType() const;
        bool HasValue() const;
        void SetValue(bool);
        void SetValue(std::string_view);

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
        void SetValue(std::enable_if_t<std::is_integral_v<T>, T> value) {
			if (this->type == SlokedCLIValue::Type::Integer) {
				this->value = SlokedCLIValue(static_cast<int64_t>(value));
			} else {
				throw SlokedError("CLIOption: Error assigning integer to " + std::string(SlokedCLIValue::TypeToName(this->type)));
			}
		}

		template <typename T>
        void SetValue(std::enable_if_t<std::is_floating_point_v<T>, T> value) {
			if (this->type == SlokedCLIValue::Type::Float) {
				this->value = SlokedCLIValue(static_cast<double>(value));
			} else {
				throw SlokedError("CLIOption: Error assigning float to " + std::string(SlokedCLIValue::TypeToName(this->type)));
			}
		}

		template <typename T>
		void SetFallback(T &&value) {
			if (!this->value.has_value()) {
				this->SetValue(std::forward<T>(value));
			}
		}

     private:
        SlokedCLIValue::Type type;
        std::optional<SlokedCLIValue> value;
    };

    class SlokedCLI {
		template <typename T, typename E = void>
		struct TypeSwitcher;

		template <typename T>
		struct TypeSwitcher<T, std::enable_if_t<std::is_integral_v<T>>> {
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
		struct TypeSwitcher<T, std::enable_if_t<std::is_same_v<T, std::string>>> {
			static constexpr auto Type = SlokedCLIValue::Type::String;
		};

     public:
        bool Has(const std::string &) const;
        bool Has(char) const;
        std::size_t Size() const;
        const SlokedCLIOption &operator[](const std::string &) const;
        const SlokedCLIOption &operator[](char) const;
        std::string_view At(std::size_t) const;
		std::vector<std::string_view>::const_iterator begin() const;
		std::vector<std::string_view>::const_iterator end() const;
        void Parse(int, const char **);

		template <typename T>
        void Define(const std::string &keys, T &&value, const std::string &description = "") {
			std::shared_ptr<SlokedCLIOption> option;
			if constexpr (std::is_integral_v<T>) {
				option = std::make_shared<SlokedCLIOption>(static_cast<int64_t>((value)));
			} else {
				option = std::make_shared<SlokedCLIOption>(std::forward<T>(value));
			}
			this->DefineImpl(keys, option);
			this->descriptions.push_back(std::make_pair(OptionDescription{keys, description}, option));
		}

		template <typename T>
		void Fallback(const std::string &keys, T &&value) {
			auto options = this->FindKeys(keys);
			for (auto &option : options) {
				option->SetFallback<T>(std::forward<T>(value));
			}
		}

		template <typename T>
		auto Option() const {
			return TypeSwitcher<T>::Type;
		}

		template <typename T>
		auto Option(T &&value) const {
			if constexpr (std::is_integral_v<T>) {
				return SlokedCLIValue(static_cast<int64_t>((value)));
			} else {
				return SlokedCLIValue(std::forward<T>(value));
			}
		}
    
     private:
        void ParseOption(std::string_view, SlokedCLIArgumentIterator &);
        void ParseShortOption(std::string_view, SlokedCLIArgumentIterator &);
		void DefineImpl(std::string_view, std::shared_ptr<SlokedCLIOption>);
		std::vector<std::shared_ptr<SlokedCLIOption>> FindKeys(std::string_view); 

		struct OptionDescription {
			std::string keys;
			std::string description;
		};

        std::map<std::string, std::shared_ptr<SlokedCLIOption>> options;
        std::map<char, std::shared_ptr<SlokedCLIOption>> shortOptions;
		std::vector<std::string_view> arguments;
		std::vector<std::pair<OptionDescription, std::shared_ptr<SlokedCLIOption>>> descriptions;
    };
}

#endif