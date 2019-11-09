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

#include "sloked/cli/Parser.h"
#include "sloked/core/String.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedCLILexer::SlokedCLILexer(std::size_t argc, const char **argv)
        : argc(argc - 1), argv(argv + 1), raw(false) {}

    bool SlokedCLILexer::HasNext() const {
        return this->argc > 0 || !this->lexems.empty();
    }

    std::pair<SlokedCLILexer::Lexem, std::string_view> SlokedCLILexer::Next() {
        if (!this->HasNext()) {
            throw SlokedError("CommandLineLexer: Expected argument");
        }
        if (!this->lexems.empty()) {
            auto lexem = this->lexems.front();
            this->lexems.pop();
            return lexem;
        }
        std::string_view arg(this->argv[0]);
        this->argc--;
        this->argv++;
        if (arg == "--" && !this->raw) {
            this->raw = true;
            return this->Next();
        } else if (starts_with(arg, "--") && !this->raw) {
            arg.remove_prefix(2);
            auto pos = arg.find('=');
            if (pos != arg.npos) {
                lexems.push(std::make_pair(Lexem::Argument, arg.substr(pos + 1)));
                return std::make_pair(Lexem::Option, arg.substr(0, pos));
            } else {
                return std::make_pair(Lexem::Option, arg);
            }
        } else if (starts_with(arg, "-") && !this->raw) {
            arg.remove_prefix(1);
            return std::make_pair(Lexem::ShortOption, arg);
        } else {
            return std::make_pair(Lexem::Argument, arg);
        }
    }

    void SlokedCLIParser::DefineOption(const std::string &option, bool flag) {
        this->options[option] = flag;
    }

    void SlokedCLIParser::DefineFlag(char shortOption, bool flag) {
        this->shortOptions[shortOption] = flag;
    }

    SlokedCLIParser::Arguments SlokedCLIParser::Parse(SlokedCLILexer &lexer) const {
        Arguments result;
        while (lexer.HasNext()) {
            auto lexem = lexer.Next();
            switch (lexem.first) {
                case Lexem::Argument:
                    result.arguments.push_back(lexem.second);
                    break;

                case Lexem::Option: {
                    std::string option{lexem.second};
                    if (this->options.count(option) == 0) {
                        throw SlokedError("CommandLineParser: Unknown option '--" + option + "'");
                    }
                    if (this->options.at(option)) {
                        result.flagOptions.insert(lexem.second);
                    } else  {
                        auto key = lexem.second;
                        result.options[key] = lexer.Next().second;
                    }
                } break;

                case Lexem::ShortOption: {
                    std::string_view key = lexem.second;
                    while (!key.empty()) {
                        char option = key.at(0);
                        key.remove_prefix(1);
                        if (this->shortOptions.count(option) == 0) {
                            throw SlokedError("CommandLineParser: Unknown option '-" + std::string(1, option) + "'");
                        }
                        if (this->shortOptions.at(option)) {
                            result.flagShortOptions.insert(option);
                        } else if (!key.empty()) {
                            result.shortOptions[option] = key;
                            break;
                        } else {
                            result.shortOptions[option] = lexer.Next().second;
                        }
                    }
                } break;
            }
        }
        return result;
    }
}