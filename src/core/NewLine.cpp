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

#include "sloked/core/NewLine.h"
#include "sloked/core/Encoding.h"
#include <algorithm>
#include <iostream>

namespace sloked {

    class LFNewLine : public NewLine {
     public:
        LFNewLine(const Encoding &enc)
            : NewLine(enc.Encode(U'\n')), encoding(enc) {}
        
        void Iterate(std::string_view str, Iterator iter) const override {
            this->encoding.IterateCodepoints(str, [&](auto start, auto length, auto chr) {
                if (chr == U'\n') {
                    iter(start, length);
                }
                return true;
            });
        }
        std::size_t Count(std::string_view str) const override {
            std::size_t count = 1;
            this->encoding.IterateCodepoints(str, [&](auto start, auto length, auto chr) {
                if (chr == U'\n') {
                    count++;
                }
                return true;
            });
            return count;
        }
    
     private:
        const Encoding &encoding;
    };

    class CRLFNewLine : public NewLine {
     public:
        CRLFNewLine(const Encoding &enc)
            : NewLine(enc.Encode(U"\r\n")), encoding(enc) {}
        
        void Iterate(std::string_view str, Iterator iter) const override {
            std::size_t last_char_pos = 0;
            char32_t last_char = U'\0';
            this->encoding.IterateCodepoints(str, [&](auto start, auto length, auto chr) {
                if (last_char == U'\r' && chr == U'\n') {
                    iter(last_char_pos, start - last_char_pos + length);
                }
                last_char = chr;
                last_char_pos = start;
                return true;
            });
        }

        std::size_t Count(std::string_view str) const override {
            std::size_t count = 1;
            char32_t last_char = U'\0';
            this->encoding.IterateCodepoints(str, [&](auto start, auto length, auto chr) {
                if (last_char == U'\r' && chr == U'\n') {
                    count++;
                }
                last_char = chr;
                return true;
            });
            return count;
        }
    
     private:
        const Encoding &encoding;
    };

    NewLine::NewLine(const std::string &symbol)
        : Width(symbol.size()), symbol(symbol) {}

    std::ostream &operator<<(std::ostream &os, const NewLine &newline) {
        return os << newline.symbol;
    }

    std::unique_ptr<NewLine> NewLine::LF(const Encoding &enc) {
        return std::make_unique<LFNewLine>(enc);
    }

    std::unique_ptr<NewLine> NewLine::CRLF(const Encoding &enc) {
        return std::make_unique<CRLFNewLine>(enc);
    }
}