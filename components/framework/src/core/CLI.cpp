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

#include "sloked/core/CLI.h"
#include "sloked/core/String.h"
#include "sloked/kgr/Path.h"
#include <set>
#include <cassert>

namespace sloked {

    SlokedCLIArgumentIterator::SlokedCLIArgumentIterator(std::size_t argc, const char **argv)
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

    SlokedCLIValue::SlokedCLIValue(int64_t value)
        : value(value) {}
        
    SlokedCLIValue::SlokedCLIValue(double value)
        : value(value) {}

    SlokedCLIValue::SlokedCLIValue(bool value)
        : value(value) {}

    SlokedCLIValue::SlokedCLIValue(std::string_view value)
        : value(value) {}

    SlokedCLIValue::SlokedCLIValue(const std::string & value)
        : value(value) {}

    SlokedCLIValue::SlokedCLIValue(const char * value)
        : value(value) {}

    SlokedCLIValue::Type SlokedCLIValue::GetType() const {
        switch (this->value.GetType()) {
            case KgrValueType::Integer:
                return Type::Integer;
            
            case KgrValueType::Number:
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
        : type(type) {}

    SlokedCLIOption::SlokedCLIOption(SlokedCLIValue value)
        : type(value.GetType()), value(std::move(value)) {}

    SlokedCLIValue::Type SlokedCLIOption::GetType() const {
        return this->type;
    }

    bool SlokedCLIOption::HasValue() const {
        return this->value.has_value();
    }

    void SlokedCLIOption::Map(const SlokedPath &path) {
        this->path = path;
    }

    bool SlokedCLIOption::Export(KgrValue &root) {
        if (this->path.has_value() && this->value.has_value()) {
            KgrPath::Assign(root, this->path.value(), this->value.value().GetValue());
            return true;
        } else {
            return false;
        }
    }

    bool SlokedCLI::Has(const std::string &key) const {
        return this->options.count(key) != 0 && this->options.at(key)->HasValue();
    }

    bool SlokedCLI::Has(char key) const {
        return this->shortOptions.count(key) != 0 && this->shortOptions.at(key)->HasValue();
    }

    std::size_t SlokedCLI::Count() const {
        return this->arguments.size();
    }

    const SlokedCLIOption &SlokedCLI::operator[](const std::string &key) const {
        if (this->options.count(key) != 0 && this->options.at(key)->HasValue()) {
            return *this->options.at(key);
        } else if (this->options.count(key) == 0) {
            throw SlokedError("CLI: Undefined option '--" + key + "'");
        } else {
            throw SlokedError("CLI: Undefined option '--" + key + "' value");
        }
    }

    const SlokedCLIOption &SlokedCLI::operator[](char key) const {
        if (this->shortOptions.count(key) != 0 && this->shortOptions.at(key)->HasValue()) {
            return *this->shortOptions.at(key);
        } else if (this->shortOptions.count(key) == 0) {
            throw SlokedError("CLI: Undefined option '-" + std::string(1, key) + "'");
        } else {
            throw SlokedError("CLI: Undefined '-" + std::string(1, key) + "' value");
        }
    }

    std::string_view SlokedCLI::At(std::size_t idx) const {
        if (idx < this->arguments.size()) {
            return this->arguments.at(idx);
        } else {
            throw SlokedError("CLI: Argument " + std::to_string(idx) + " is out of range");
        }
    }

    std::vector<std::string_view>::const_iterator SlokedCLI::begin() const {
        return this->arguments.begin();
    }

    std::vector<std::string_view>::const_iterator SlokedCLI::end() const {
        return this->arguments.end();
    }

    void SlokedCLI::Parse(int argc, const char **argv) {
        SlokedCLIArgumentIterator args(argc - 1, argv + 1);
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

    static bool ParseOptionValue(SlokedCLIOption &option, std::string_view arg, SlokedCLIArgumentIterator &args) {
            switch (option.GetType()) {
                case SlokedCLIValue::Type::Integer: {
                    std::string value{!arg.empty() ? arg : args.Next()};
                    long long int_value;
                    try {
                        int_value = std::stoll(value);
                    } catch (const std::invalid_argument &ex) {
                        throw SlokedError("CLI: Error converting '" + value + "' to integer");
                    }
                    option.SetValue(int_value);
                } break;

                case SlokedCLIValue::Type::Float: {
                    std::string value{!arg.empty() ? arg : args.Next()};
                    double float_value;
                    try {
                        float_value = std::stod(value);
                    } catch (const std::invalid_argument &ex) {
                        throw SlokedError("CLI: Error converting '" + value + "' to float");
                    }
                    option.SetValue(float_value);
                } break;

                case SlokedCLIValue::Type::Boolean:
                    option.SetValue(true);
                    return true;

                case SlokedCLIValue::Type::String: {
                    std::string value{!arg.empty() ? arg : args.Next()};
                    option.SetValue(value);
                    return false;
                } break;
            }
            return false;
    }

    void SlokedCLI::ParseOption(std::string_view arg, SlokedCLIArgumentIterator &args) {
        auto pos = arg.find('=');
        std::string key{pos != arg.npos ? arg.substr(0, pos) : arg};
        arg.remove_prefix(key.size());
        if (pos != arg.npos) {
            arg.remove_prefix(1);
        }

        if (this->options.count(key) == 0) {
            throw SlokedError("CLI: Unknown option '--" + key + "'");
        }
        auto &option = *this->options.at(key);
        ParseOptionValue(option, arg, args);
    }

    void SlokedCLI::ParseShortOption(std::string_view arg, SlokedCLIArgumentIterator &args) {
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

    void SlokedCLI::DefineImpl(std::string_view allKeys, std::shared_ptr<SlokedCLIOption> option) {
        while (!allKeys.empty()) {
            auto keyEnd = allKeys.find(',');
            auto key = allKeys.substr(0, keyEnd != allKeys.npos ? keyEnd : allKeys.size());
            allKeys.remove_prefix(keyEnd != allKeys.npos ? keyEnd + 1 : allKeys.size());
            if (starts_with(key, "--")) {
                key.remove_prefix(2);
                std::string optionKey{key};
                if (this->options.count(optionKey) == 0) {
                    this->options.emplace(optionKey, option);
                } else {
                    throw SlokedError("CLI: Duplicate option '--" + optionKey + "'");
                }
            } else if (starts_with(key, "-") && key.size() == 2) {
                if (this->shortOptions.count(key[1]) == 0) {
                    this->shortOptions.emplace(key[1], option);
                } else {
                    throw SlokedError("CLI: Duplicate option '" + std::string(1, key[1]) + "'");
                }
            } else {
                throw SlokedError("CLI: Malformed option definition " + std::string(key));
            }
        }
    }

    std::vector<std::shared_ptr<SlokedCLIOption>> SlokedCLI::FindKeys(std::string_view allKeys) {
        std::vector<std::shared_ptr<SlokedCLIOption>> result;
        while (!allKeys.empty()) {
            auto keyEnd = allKeys.find(',');
            auto key = allKeys.substr(0, keyEnd != allKeys.npos ? keyEnd : allKeys.size());
            allKeys.remove_prefix(keyEnd != allKeys.npos ? keyEnd + 1 : allKeys.size());
            if (starts_with(key, "--")) {
                key.remove_prefix(2);
                std::string optionKey{key};
                if (this->options.count(optionKey) != 0) {
                    result.push_back(this->options.at(optionKey));
                } else {
                    throw SlokedError("CLI: Unknown option '--" + optionKey + "'");
                }
            } else if (starts_with(key, "-") && key.size() == 2) {
                if (this->shortOptions.count(key[1]) != 0) {
                   result.push_back(this->shortOptions.at(key[1]));
                } else {
                    throw SlokedError("CLI: Unknown option '" + std::string(1, key[1]) + "'");
                }
            } else {
                throw SlokedError("CLI: Malformed option definition " + std::string(key));
            }
        }
        return result;
    }
}