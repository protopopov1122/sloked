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

#ifndef SLOKED_CLI_PARSER_H_
#define SLOKED_CLI_PARSER_H_

#include "sloked/Base.h"
#include <map>
#include <set>
#include <vector>
#include <string>
#include <queue>

namespace sloked {

    class SlokedCLILexer {
     public:
        enum class Lexem {
            Option,
            ShortOption,
            Argument
        };

        SlokedCLILexer(std::size_t, const char **);
        bool HasNext() const;
        std::pair<Lexem, std::string_view> Next();

     private:
        std::size_t argc;
        const char **argv;
        bool raw;
        std::queue<std::pair<Lexem, std::string_view>> lexems;
    };

    class SlokedCLIParser {
     public:
        using Lexem = SlokedCLILexer::Lexem;

        struct Arguments {
            std::set<std::string_view> flagOptions;
            std::map<std::string_view, std::string_view> options;
            std::set<char> flagShortOptions;
            std::map<char, std::string_view> shortOptions;
            std::vector<std::string_view> arguments;
        };

        void DefineOption(const std::string &, bool);
        void DefineFlag(char, bool);
        Arguments Parse(SlokedCLILexer &) const;

     private:
        std::map<std::string, bool> options;
        std::map<char, bool> shortOptions;
    };
}

#endif