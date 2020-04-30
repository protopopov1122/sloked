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

#include "sloked/core/Base64.h"

#include <algorithm>

namespace sloked {

    std::string SlokedBase64::Encode(Iterator begin, const Iterator end) {
        constexpr auto Chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        constexpr auto Pad = '=';
        std::string encoded;
        const std::size_t length = std::distance(begin, end);
        encoded.reserve(((length / 3) + (length % 3 > 0)) * 4);
        uint32_t buffer;
        std::size_t i = 0;
        for (; length >= 2 && i < length - 2;) {
            buffer = begin[i++] << 16;
            buffer |= begin[i++] << 8;
            buffer |= begin[i++];
            encoded.push_back(Chars[(buffer & 0x00FC0000) >> 18]);
            encoded.push_back(Chars[(buffer & 0x0003F000) >> 12]);
            encoded.push_back(Chars[(buffer & 0x00000FC0) >> 6]);
            encoded.push_back(Chars[buffer & 0x0000003F]);
        }
        switch (length % 3) {
            case 1:
                buffer = begin[i] << 16;
                encoded.push_back(Chars[(buffer & 0x00FC0000) >> 18]);
                encoded.push_back(Chars[(buffer & 0x0003F000) >> 12]);
                encoded.push_back(Pad);
                encoded.push_back(Pad);
                break;
            case 2:
                buffer = begin[i++] << 16;
                buffer |= begin[i] << 8;
                encoded.push_back(Chars[(buffer & 0x00FC0000) >> 18]);
                encoded.push_back(Chars[(buffer & 0x0003F000) >> 12]);
                encoded.push_back(Chars[(buffer & 0x00000FC0) >> 6]);
                encoded.push_back(Pad);
                break;
        }
        return encoded;
    }

    std::vector<SlokedBase64::Byte> SlokedBase64::Decode(
        std::string_view encoded) {
        constexpr auto Pad = '=';
        std::size_t padding = 0;
        if (encoded.size() > 0) {
            if (encoded[encoded.size() - 1] == Pad) {
                padding++;
            }
            if (encoded[encoded.size() - 2] == Pad) {
                padding++;
            }
        }
        std::vector<SlokedBase64::Byte> data;
        data.reserve(((encoded.size() / 4) * 3) - padding);
        std::size_t idx = 0;
        uint32_t buffer{0};
        for (const auto chr : encoded) {
            buffer <<= 6;
            switch (chr) {
                // Upper-case letters
                case 'A':
                    buffer |= 0;
                    break;
                case 'B':
                    buffer |= 1;
                    break;
                case 'C':
                    buffer |= 2;
                    break;
                case 'D':
                    buffer |= 3;
                    break;
                case 'E':
                    buffer |= 4;
                    break;
                case 'F':
                    buffer |= 5;
                    break;
                case 'G':
                    buffer |= 6;
                    break;
                case 'H':
                    buffer |= 7;
                    break;
                case 'I':
                    buffer |= 8;
                    break;
                case 'J':
                    buffer |= 9;
                    break;
                case 'K':
                    buffer |= 10;
                    break;
                case 'L':
                    buffer |= 11;
                    break;
                case 'M':
                    buffer |= 12;
                    break;
                case 'N':
                    buffer |= 13;
                    break;
                case 'O':
                    buffer |= 14;
                    break;
                case 'P':
                    buffer |= 15;
                    break;
                case 'Q':
                    buffer |= 16;
                    break;
                case 'R':
                    buffer |= 17;
                    break;
                case 'S':
                    buffer |= 18;
                    break;
                case 'T':
                    buffer |= 19;
                    break;
                case 'U':
                    buffer |= 20;
                    break;
                case 'V':
                    buffer |= 21;
                    break;
                case 'W':
                    buffer |= 22;
                    break;
                case 'X':
                    buffer |= 23;
                    break;
                case 'Y':
                    buffer |= 24;
                    break;
                case 'Z':
                    buffer |= 25;
                    break;
                // Lower-case letters
                case 'a':
                    buffer |= 26;
                    break;
                case 'b':
                    buffer |= 27;
                    break;
                case 'c':
                    buffer |= 28;
                    break;
                case 'd':
                    buffer |= 29;
                    break;
                case 'e':
                    buffer |= 30;
                    break;
                case 'f':
                    buffer |= 31;
                    break;
                case 'g':
                    buffer |= 32;
                    break;
                case 'h':
                    buffer |= 33;
                    break;
                case 'i':
                    buffer |= 34;
                    break;
                case 'j':
                    buffer |= 35;
                    break;
                case 'k':
                    buffer |= 36;
                    break;
                case 'l':
                    buffer |= 37;
                    break;
                case 'm':
                    buffer |= 38;
                    break;
                case 'n':
                    buffer |= 39;
                    break;
                case 'o':
                    buffer |= 40;
                    break;
                case 'p':
                    buffer |= 41;
                    break;
                case 'q':
                    buffer |= 42;
                    break;
                case 'r':
                    buffer |= 43;
                    break;
                case 's':
                    buffer |= 44;
                    break;
                case 't':
                    buffer |= 45;
                    break;
                case 'u':
                    buffer |= 46;
                    break;
                case 'v':
                    buffer |= 47;
                    break;
                case 'w':
                    buffer |= 48;
                    break;
                case 'x':
                    buffer |= 49;
                    break;
                case 'y':
                    buffer |= 50;
                    break;
                case 'z':
                    buffer |= 51;
                    break;
                // Digits
                case '0':
                    buffer |= 52;
                    break;
                case '1':
                    buffer |= 53;
                    break;
                case '2':
                    buffer |= 54;
                    break;
                case '3':
                    buffer |= 55;
                    break;
                case '4':
                    buffer |= 56;
                    break;
                case '5':
                    buffer |= 57;
                    break;
                case '6':
                    buffer |= 58;
                    break;
                case '7':
                    buffer |= 59;
                    break;
                case '8':
                    buffer |= 60;
                    break;
                case '9':
                    buffer |= 61;
                    break;
                // Other
                case '+':
                    buffer |= 62;
                    break;
                case '/':
                    buffer |= 63;
                    break;
                // Pad
                case '=':
                    switch (encoded.size() - idx) {
                        case 1:
                            data.push_back((buffer >> 16) & 0xff);
                            data.push_back((buffer >> 8) & 0xff);
                            return data;
                        case 2:
                            data.push_back((buffer >> 10) & 0xff);
                            return data;
                    }
                    break;
            }
            if (++idx % 4 == 0) {
                data.push_back((buffer >> 16) & 0xff);
                data.push_back((buffer >> 8) & 0xff);
                data.push_back(buffer & 0xff);
                buffer = 0;
            }
        }
        return data;
    }
}  // namespace sloked