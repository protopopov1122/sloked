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

#include "sloked/core/CLI.h"

#include <cassert>
#include <set>
#include <stdexcept>

#include "sloked/core/String.h"
#include "sloked/kgr/Path.h"

namespace sloked {

    SlokedCLIArgumentIterator::SlokedCLIArgumentIterator(std::size_t argc,
                                                         const char **argv)
        : argc(argc), argv(argv) {}

    bool SlokedCLIArgumentIterator::HasNext() const {
        return this->argc > 0;
    }

    std::string_view SlokedCLIArgumentIterator::Next() {
        if (this->argc > 0) {
            this->argc--;
            return std::string_view(*(this->argv++));
        } else {
            throw SlokedError("CLIArguments: No more arguments");
        }
    }

    SlokedCLIValue::SlokedCLIValue(int64_t value) : value(value) {}

    SlokedCLIValue::SlokedCLIValue(double value) : value(value) {}

    SlokedCLIValue::SlokedCLIValue(bool value) : value(value) {}

    SlokedCLIValue::SlokedCLIValue(std::string_view value) : value(value) {}

    SlokedCLIValue::SlokedCLIValue(const std::string &value) : value(value) {}

    SlokedCLIValue::SlokedCLIValue(const char *value) : value(value) {}

    SlokedCLIValue::Type SlokedCLIValue::GetType() const {
        switch (this->value.GetType()) {
            case KgrValueType::Integer:
                return Type::Integer;

            case KgrValueType::Float:
                return Type::Float;

            case KgrValueType::Boolean:
                return Type::Boolean;

            case KgrValueType::String:
                return Type::String;

            default:
                assert(false);
                break;
        }
    }

    const KgrValue &SlokedCLIValue::GetValue() const {
        return this->value;
    }

    SlokedCLIOption::SlokedCLIOption(SlokedCLIValue::Type type)
        : type(type), mandatory{false} {}

    SlokedCLIOption::SlokedCLIOption(SlokedCLIValue value)
        : type(value.GetType()), value(std::move(value)), mandatory{false} {}

    SlokedCLIValue::Type SlokedCLIOption::Type() const {
        return this->type;
    }

    bool SlokedCLIOption::Empty() const {
        return !this->value.has_value();
    }

    void SlokedCLIOption::Map(const SlokedPath &path) {
        this->path = path;
    }

    bool SlokedCLIOption::Export(KgrValue &root) {
        if (this->path.has_value() && this->value.has_value()) {
            KgrPath::Assign(root, this->path.value(),
                            this->value.value().GetValue());
            return true;
        } else {
            return false;
        }
    }

    bool SlokedCLIOption::IsMandatory() const {
        return this->mandatory;
    }

    SlokedCLIOption &SlokedCLIOption::Mandatory(bool mandatory) {
        this->mandatory = mandatory;
        return *this;
    }

    bool SlokedCLI::Has(const std::string &key) const {
        return this->options.count(key) != 0 && !this->options.at(key)->Empty();
    }

    bool SlokedCLI::Has(char key) const {
        return this->shortOptions.count(key) != 0 &&
               !this->shortOptions.at(key)->Empty();
    }

    std::size_t SlokedCLI::ArgCount() const {
        return this->arguments.size();
    }

    const SlokedCLIOption &SlokedCLI::operator[](const std::string &key) const {
        if (this->options.count(key) != 0 && !this->options.at(key)->Empty()) {
            return *this->options.at(key);
        } else if (this->options.count(key) == 0) {
            throw SlokedError("CLI: Undefined option '--" + key + "'");
        } else {
            throw SlokedError("CLI: Undefined option '--" + key + "' value");
        }
    }

    const SlokedCLIOption &SlokedCLI::operator[](char key) const {
        if (this->shortOptions.count(key) != 0 &&
            !this->shortOptions.at(key)->Empty()) {
            return *this->shortOptions.at(key);
        } else if (this->shortOptions.count(key) == 0) {
            throw SlokedError("CLI: Undefined option '-" + std::string(1, key) +
                              "'");
        } else {
            throw SlokedError("CLI: Undefined '-" + std::string(1, key) +
                              "' value");
        }
    }

    std::string_view SlokedCLI::Argument(std::size_t idx) const {
        if (idx < this->arguments.size()) {
            return this->arguments.at(idx);
        } else {
            throw SlokedError("CLI: Argument " + std::to_string(idx) +
                              " is out of range");
        }
    }

    std::vector<std::string_view>::const_iterator SlokedCLI::begin() const {
        return this->arguments.begin();
    }

    std::vector<std::string_view>::const_iterator SlokedCLI::end() const {
        return this->arguments.end();
    }

    void SlokedCLI::Parse(int argc, const char **argv, bool ignoreUnknowns) {
        SlokedCLIArgumentIterator args(argc, argv);
        bool rawMode = false;
        while (args.HasNext()) {
            auto arg = args.Next();
            if (arg == "--") {
                rawMode = true;
            } else if (starts_with(arg, "--") && !rawMode) {
                arg.remove_prefix(2);
                this->ParseOption(arg, args, ignoreUnknowns);
            } else if (starts_with(arg, "-") && !rawMode) {
                arg.remove_prefix(1);
                this->ParseShortOption(arg, args, ignoreUnknowns);
            } else {
                this->arguments.push_back(arg);
            }
        }
        // Check mandatory options
        for (auto &kv : this->options) {
            if (kv.second->IsMandatory() && kv.second->Empty()) {
                throw SlokedError("CLI: Mandatory \'--" + kv.first +
                                  "\' is not defined");
            }
        }
        for (auto &kv : this->shortOptions) {
            if (kv.second->IsMandatory() && kv.second->Empty()) {
                throw SlokedError("CLI: Mandatory \'-" +
                                  std::string{1, kv.first} +
                                  "\' is not defined");
            }
        }
    }

    KgrValue SlokedCLI::Export() const {
        KgrValue root = KgrDictionary{};
        std::set<SlokedCLIOption *> processed;
        for (auto &kv : this->shortOptions) {
            if (processed.count(kv.second.get()) == 0) {
                kv.second->Export(root);
                processed.insert(kv.second.get());
            }
        }
        for (auto &kv : this->options) {
            if (processed.count(kv.second.get()) == 0) {
                kv.second->Export(root);
                processed.insert(kv.second.get());
            }
        }
        return root;
    }

    void SlokedCLI::Initialize(const KgrValue &config) {
        const auto &options = config.AsArray();
        for (const auto &option : options) {
            const auto &list = option.AsDictionary()["options"].AsString();
            auto value = ([&] {
                if (option.AsDictionary().Has("value")) {
                    const auto &val = option.AsDictionary()["value"];
                    switch (val.GetType()) {
                        case KgrValueType::Integer:
                            return SlokedCLIOption{
                                this->Option<int64_t>(val.AsInt())};

                        case KgrValueType::Float:
                            return SlokedCLIOption{
                                this->Option<double>(val.AsFloat())};

                        case KgrValueType::Boolean:
                            return SlokedCLIOption{
                                this->Option<bool>(val.AsBoolean())};

                        case KgrValueType::String:
                            return SlokedCLIOption{this->Option<std::string>(
                                std::string{val.AsString()})};

                        default:
                            throw SlokedError(
                                "CLI: Unsupported default value of \'" + list +
                                "\'");
                    }
                } else {
                    const auto &strType =
                        option.AsDictionary()["type"].AsString();
                    if (strType == "int") {
                        return SlokedCLIOption{this->Option<int64_t>()};
                    } else if (strType == "number") {
                        return SlokedCLIOption{this->Option<double>()};
                    } else if (strType == "boolean") {
                        return SlokedCLIOption{this->Option<bool>()};
                    } else if (strType == "string") {
                        return SlokedCLIOption{this->Option<std::string>()};
                    } else {
                        throw SlokedError("CLI: Unsupported option type \'" +
                                          list + "\'");
                    }
                }
            })();
            std::string descr{};
            if (option.AsDictionary().Has("description")) {
                descr = option.AsDictionary()["description"].AsString();
            }
            auto &opt = this->Define(list, value, descr);
            if (option.AsDictionary().Has("mandatory")) {
                opt.Mandatory(option.AsDictionary()["mandatory"].AsBoolean());
            }
            if (option.AsDictionary().Has("map")) {
                opt.Map(SlokedPath{option.AsDictionary()["map"].AsString()});
            }
        }
    }

    static bool ParseOptionValue(SlokedCLIOption &option, std::string_view arg,
                                 SlokedCLIArgumentIterator &args) {
        switch (option.Type()) {
            case SlokedCLIValue::Type::Integer: {
                std::string value{!arg.empty() ? arg : args.Next()};
                long long int_value;
                try {
                    int_value = std::stoll(value);
                } catch (const std::invalid_argument &ex) {
                    throw SlokedError("CLI: Error converting '" + value +
                                      "' to integer");
                }
                option.Set(int_value);
            } break;

            case SlokedCLIValue::Type::Float: {
                std::string value{!arg.empty() ? arg : args.Next()};
                double float_value;
                try {
                    float_value = std::stod(value);
                } catch (const std::invalid_argument &ex) {
                    throw SlokedError("CLI: Error converting '" + value +
                                      "' to float");
                }
                option.Set(float_value);
            } break;

            case SlokedCLIValue::Type::Boolean:
                option.Set(true);
                return true;

            case SlokedCLIValue::Type::String: {
                std::string value{!arg.empty() ? arg : args.Next()};
                option.Set(value);
                return false;
            } break;
        }
        return false;
    }

    void SlokedCLI::ParseOption(std::string_view arg,
                                SlokedCLIArgumentIterator &args,
                                bool ignoreUnknowns) {
        auto pos = arg.find('=');
        std::string key{pos != arg.npos ? arg.substr(0, pos) : arg};
        arg.remove_prefix(key.size());
        if (pos != arg.npos) {
            arg.remove_prefix(1);
        }

        if (this->options.count(key) == 0) {
            if (ignoreUnknowns) {
                return;
            } else {
                throw SlokedError("CLI: Unknown option '--" + key + "'");
            }
        }
        auto &option = *this->options.at(key);
        ParseOptionValue(option, arg, args);
    }

    void SlokedCLI::ParseShortOption(std::string_view arg,
                                     SlokedCLIArgumentIterator &args,
                                     bool ignoreUnknowns) {
        while (!arg.empty()) {
            char key = arg[0];
            arg.remove_prefix(1);
            if (this->shortOptions.count(key) == 0) {
                if (ignoreUnknowns) {
                    break;
                } else {
                    throw SlokedError("CLI: Unknown option '-" +
                                      std::string(1, key) + "'");
                }
            }
            auto &option = *this->shortOptions.at(key);
            if (!ParseOptionValue(option, arg, args)) {
                break;
            }
        }
    }

    void SlokedCLI::DefineImpl(std::string_view allKeys,
                               std::shared_ptr<SlokedCLIOption> option) {
        while (!allKeys.empty()) {
            auto keyEnd = allKeys.find(',');
            auto key = allKeys.substr(
                0, keyEnd != allKeys.npos ? keyEnd : allKeys.size());
            allKeys.remove_prefix(keyEnd != allKeys.npos ? keyEnd + 1
                                                         : allKeys.size());
            if (starts_with(key, "--")) {
                key.remove_prefix(2);
                std::string optionKey{key};
                if (this->options.count(optionKey) == 0) {
                    this->options.emplace(optionKey, option);
                } else {
                    throw SlokedError("CLI: Duplicate option '--" + optionKey +
                                      "'");
                }
            } else if (starts_with(key, "-") && key.size() == 2) {
                if (this->shortOptions.count(key[1]) == 0) {
                    this->shortOptions.emplace(key[1], option);
                } else {
                    throw SlokedError("CLI: Duplicate option '" +
                                      std::string(1, key[1]) + "'");
                }
            } else {
                throw SlokedError("CLI: Malformed option definition " +
                                  std::string(key));
            }
        }
    }

    std::vector<std::shared_ptr<SlokedCLIOption>> SlokedCLI::FindKeys(
        std::string_view allKeys) {
        std::vector<std::shared_ptr<SlokedCLIOption>> result;
        while (!allKeys.empty()) {
            auto keyEnd = allKeys.find(',');
            auto key = allKeys.substr(
                0, keyEnd != allKeys.npos ? keyEnd : allKeys.size());
            allKeys.remove_prefix(keyEnd != allKeys.npos ? keyEnd + 1
                                                         : allKeys.size());
            if (starts_with(key, "--")) {
                key.remove_prefix(2);
                std::string optionKey{key};
                if (this->options.count(optionKey) != 0) {
                    result.push_back(this->options.at(optionKey));
                } else {
                    throw SlokedError("CLI: Unknown option '--" + optionKey +
                                      "'");
                }
            } else if (starts_with(key, "-") && key.size() == 2) {
                if (this->shortOptions.count(key[1]) != 0) {
                    result.push_back(this->shortOptions.at(key[1]));
                } else {
                    throw SlokedError("CLI: Unknown option '" +
                                      std::string(1, key[1]) + "'");
                }
            } else {
                throw SlokedError("CLI: Malformed option definition " +
                                  std::string(key));
            }
        }
        return result;
    }
}  // namespace sloked