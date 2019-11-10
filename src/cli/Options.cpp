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

#include "sloked/cli/Options.h"
#include "sloked/core/String.h"
#include <cassert>

namespace sloked {

    SlokedCLIArguments::SlokedCLIArguments(std::size_t argc, const char **argv)
        : argc(argc), argv(argv) {}

    bool SlokedCLIArguments::HasNext() const {
        return this->argc > 0;
    }

    std::string_view SlokedCLIArguments::Next() {
        if (this->argc > 0) {
            this->argc--;
            return std::string_view(*(this->argv++));
        } else {
            throw SlokedError("CLIArguments: No more arguments");
        }
    }

    SlokedCLIValue::SlokedCLIValue(int64_t value)
        : value(value) {}

    SlokedCLIValue::SlokedCLIValue(double value)
        : value(value) {}
        
    SlokedCLIValue::SlokedCLIValue(bool value)
        : value(value) {}

    SlokedCLIValue::SlokedCLIValue(std::string_view value)
        : value(std::string{value}) {}

    SlokedCLIValue::SlokedCLIValue(const std::string &value)
        : value(value) {}

    SlokedCLIValue::Type SlokedCLIValue::GetType() const {
        switch (this->value.index()) {
            case 0:
                return Type::Integer;

            case 1:
                return Type::Float;

            case 2:
                return Type::Boolean;

            case 3:
                return Type::String;

            default:
                assert(false);
        }
    }

    bool SlokedCLIValue::Is(Type type) const {
        switch (type) {
            case Type::Integer:
                return this->value.index() == 0;

            case Type::Float:
                return this->value.index() == 1;

            case Type::Boolean:
                return this->value.index() == 2;
                
            case Type::String:
                return this->value.index() == 3;
        }
        return false;
    }

    int64_t SlokedCLIValue::AsInt() const {
        if (this->value.index() == 0) {
            return std::get<0>(this->value);
        } else {
            throw SlokedError("CLIValue: Not an integer");
        }
    }

    double SlokedCLIValue::AsFloat() const {
        if (this->value.index() == 1) {
            return std::get<1>(this->value);
        } else {
            throw SlokedError("CLIValue: Not a float");
        }
    }

    bool SlokedCLIValue::AsBoolean() const {
        if (this->value.index() == 2) {
            return std::get<2>(this->value);
        } else {
            throw SlokedError("CLIValue: Not a boolean");
        }
    }

    const std::string &SlokedCLIValue::AsString() const {
        if (this->value.index() == 3) {
            return std::get<3>(this->value);
        } else {
            throw SlokedError("CLIValue: Not a string");
        }
    }

    SlokedCLIOption::SlokedCLIOption(SlokedCLIValue::Type type)
        : type(type) {}

    SlokedCLIOption::SlokedCLIOption(SlokedCLIValue &&value)
        : type(value.GetType()), value(std::forward<SlokedCLIValue>(value)) {}

    SlokedCLIValue::Type SlokedCLIOption::GetType() const {
        return this->type;
    }

    bool SlokedCLIOption::HasValue() const {
        return this->value.has_value();
    }

    int64_t SlokedCLIOption::AsInt() const {
        if (this->value.has_value()) {
            return this->value.value().AsInt();
        } else {
            throw SlokedError("CLIOption: Empty");
        }
    }

    double SlokedCLIOption::AsFloat() const {
        if (this->value.has_value()) {
            return this->value.value().AsFloat();
        } else {
            throw SlokedError("CLIOption: Empty");
        }
    }

    bool SlokedCLIOption::AsBoolean() const {
        if (this->value.has_value()) {
            return this->value.value().AsBoolean();
        } else {
            throw SlokedError("CLIOption: Empty");
        }
    }

    const std::string &SlokedCLIOption::AsString() const {
        if (this->value.has_value()) {
            return this->value.value().AsString();
        } else {
            throw SlokedError("CLIOption: Empty");
        }
    }

    void SlokedCLIOption::SetValue(int64_t value) {
        if (this->type == SlokedCLIValue::Type::Integer) {
            this->value = SlokedCLIValue(value);
        } else {
            throw SlokedError("CLIOption: Not an integer");
        }
    }

    void SlokedCLIOption::SetValue(double value) {
        if (this->type == SlokedCLIValue::Type::Float) {
            this->value = SlokedCLIValue(value);
        } else {
            throw SlokedError("CLIOption: Not a float");
        }
    }

    void SlokedCLIOption::SetValue(bool value) {
        if (this->type == SlokedCLIValue::Type::Boolean) {
            this->value = SlokedCLIValue(value);
        } else {
            throw SlokedError("CLIOption: Not a boolean");
        }
    }

    void SlokedCLIOption::SetValue(std::string_view value) {
        if (this->type == SlokedCLIValue::Type::String) {
            this->value = SlokedCLIValue(value);
        } else {
            throw SlokedError("CLIOption: Not a string");
        }
    }

    bool SlokedCLI::Has(const std::string &key) const {
        return this->options.count(key) != 0 && this->options.at(key).HasValue();
    }

    bool SlokedCLI::Has(char key) const {
        return this->shortOptions.count(key) != 0 && this->shortOptions.at(key)->HasValue();
    }

    std::size_t SlokedCLI::ArgCount() const {
        return this->arguments.size();
    }

    const SlokedCLIOption &SlokedCLI::operator[](const std::string &key) const {
        if (this->options.count(key) != 0) {
            return this->options.at(key);
        } else {
            throw SlokedError("CLI: Undefined option '--" + key + "'");
        }
    }

    const SlokedCLIOption &SlokedCLI::operator[](char key) const {
        if (this->shortOptions.count(key) != 0) {
            return *this->shortOptions.at(key);
        } else {
            throw SlokedError("CLI: Undefined option '-" + std::string(1, key) + "'");
        }
    }

    std::string_view SlokedCLI::At(std::size_t idx) const {
        if (idx < this->arguments.size()) {
            return this->arguments.at(idx);
        } else {
            throw SlokedError("CLI: Unknown argument " + std::to_string(idx));
        }
    }

    void SlokedCLI::Define(const std::string &key, SlokedCLIOption &&option) {
        if (this->options.count(key) == 0) {
            this->options.emplace(key, std::forward<SlokedCLIOption>(option));
        } else {
            throw SlokedError("CLI: Duplicate option '--" + key + "'");
        }
    }

    void SlokedCLI::Define(char shortKey, const std::string &key, SlokedCLIOption &&option) {
        if (this->shortOptions.count(shortKey) == 0 && this->options.count(key) == 0) {
            this->options.emplace(key, std::forward<SlokedCLIOption>(option));
            this->shortOptions.emplace(shortKey, std::addressof(this->options.at(key)));
        } else if (this->shortOptions.count(shortKey) != 0) {
            throw SlokedError("CLI: Duplicate option '-" + std::string(1, shortKey) + "'");
        } else {
            throw SlokedError("CLI: Duplicate option '--" + key + "'");
        }
    }
    void SlokedCLI::Parse(SlokedCLIArguments &args) {
        bool rawMode = false;
        while (args.HasNext()) {
            auto arg = args.Next();
            if (arg == "--") {
                rawMode = true;
            } else if (starts_with(arg, "--") && !rawMode) {
                arg.remove_prefix(2);
                this->ParseOption(arg, args);
            } else if (starts_with(arg, "-") && !rawMode) {
                arg.remove_prefix(1);
                this->ParseShortOption(arg, args);
            } else {
                this->arguments.push_back(arg);
            }
        }
    }

    static bool ParseOptionValue(SlokedCLIOption &option, std::string_view arg, SlokedCLIArguments &args) {
        if (option.GetType() == SlokedCLIValue::Type::Boolean) {
            option.SetValue(true);
            return true;
        } else {
            std::string value;
            if (!arg.empty()) {
                value = arg;
            } else {
                value = args.Next();
            }
            switch (option.GetType()) {
                case SlokedCLIValue::Type::Integer:
                    option.SetValue(static_cast<int64_t>(std::stoll(value)));
                    break;

                case SlokedCLIValue::Type::Float:
                    option.SetValue(std::stod(value));
                    break;

                case SlokedCLIValue::Type::String:
                    option.SetValue(value);
                    break;

                default:
                    break;
            }
            return false;
        }
    }

    void SlokedCLI::ParseOption(std::string_view arg, SlokedCLIArguments &args) {
        auto pos = arg.find('=');
        std::string key;
        if (pos != arg.npos) {
            key = arg.substr(0, pos);
        } else {
            key = arg;
        }
        arg.remove_prefix(key.size());

        if (this->options.count(key) == 0) {
            throw SlokedError("CLI: Unknown option '--" + key + "'");
        }
        auto &option = this->options.at(key);
        ParseOptionValue(option, arg, args);
    }

    void SlokedCLI::ParseShortOption(std::string_view arg, SlokedCLIArguments &args) {
        while (!arg.empty()) {
            char key = arg[0];
            arg.remove_prefix(1);
            if (this->shortOptions.count(key) == 0) {
                throw SlokedError("CLI: Unknown option '-" + std::string(1, key) + "'");
            }
            auto &option = *this->shortOptions.at(key);
            if (!ParseOptionValue(option, arg, args)) {
                break;
            }
        }
    }
}