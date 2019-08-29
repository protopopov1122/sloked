/*
  SPDX-License-Identifier: LGPL-3.0-or-later

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/core/Encoding.h"
#include "sloked/core/Error.h"
#include <iostream>

namespace sloked {

    std::u32string Encoding::Decode(std::string_view view) const {
        std::u32string res;
        this->IterateCodepoints(view, [&](auto start, auto length, auto chr) {
            res.push_back(chr);
            return true;
        });
        return res;
    }

    static size_t Char32ToUtf8(char *const buffer, const char32_t code) {
        if (code <= 0x7F) {
            buffer[0] = code;
            return 1;
        } else if (code <= 0x7FF) {
            buffer[0] = 0xC0 | (code >> 6);
            buffer[1] = 0x80 | (code & 0x3F);
            return 2;
        } else if (code <= 0xFFFF) {
            buffer[0] = 0xE0 | (code >> 12);
            buffer[1] = 0x80 | ((code >> 6) & 0x3F);
            buffer[2] = 0x80 | (code & 0x3F);
            return 3;
        } else if (code <= 0x10FFFF) {
            buffer[0] = 0xF0 | (code >> 18);
            buffer[1] = 0x80 | ((code >> 12) & 0x3F);
            buffer[2] = 0x80 | ((code >> 6) & 0x3F);
            buffer[3] = 0x80 | (code & 0x3F);
            return 4;
        } else {
            throw SlokedError("UTF-8: Invalid codepoint");
        }
    }

    class Utf8Encoding : public Encoding {
     public:
        std::size_t CodepointCount(std::string_view str) const override {
            std::size_t count = 0;
            for (std::size_t i = 0; i < str.size(); i++) {
                char current = str[i];
                count++;
                if ((current & 0xc0) != 0xc0) {
                } else if ((current & 0xe0) != 0xe0) {
                    i++;
                } else if ((current & 0xf0) != 0xf0) {
                    i += 2;
                } else {
                    i += 3;
                }
            }
            return count;
        }

        std::pair<std::size_t, std::size_t> GetCodepoint(std::string_view str, std::size_t idx) const override {
#define ASSERT_WIDTH(x) do { \
                if (i + (x) - 1 >= str.size()) { \
                    return std::make_pair(0, 0); \
                } \
            } while (false)
            for (std::size_t i = 0; i < str.size();) {
                char current = str[i];
                std::size_t width = 0;
                if ((current & 0xc0) != 0xc0){
                    width = 1;
                    ASSERT_WIDTH(width);
                } else if ((current & 0xe0) != 0xe0) {
                    width = 2;
                    ASSERT_WIDTH(width);
                } else if ((current & 0xf0) != 0xf0) {
                    width = 3;
                    ASSERT_WIDTH(width);
                } else {
                    width = 4;
                    ASSERT_WIDTH(width);
                }
                if (!idx--) {
                    return std::make_pair(i, width);
                } else {
                    i += width;
                }
            }
#undef ASSERT_WIDTH
            return std::make_pair(0, 0);
        }

        bool IterateCodepoints(std::string_view str, std::function<bool(std::size_t, std::size_t, char32_t)> callback) const override {
#define ASSERT_WIDTH(x) do { \
                            if (i + (x) - 1 >= str.size()) { \
                                return false; \
                            } \
                        } while (false)
            bool res = true;
            for (std::size_t i = 0; i < str.size() && res;) {
                char32_t result;
                std::size_t width = 0;
                char current = str[i];
                if ((current & 0xc0) != 0xc0){
                    width = 1;
                    ASSERT_WIDTH(width);
                    result = current;
                } else if ((current & 0xe0) != 0xe0) {
                    width = 2;
                    ASSERT_WIDTH(width);
                    result = ((current & 0x1f) << 6)
                        | (str[i + 1] & 0x3f);
                } else if ((current & 0xf0) != 0xf0) {
                    width = 3;
                    ASSERT_WIDTH(width);
                    result = ((current & 0xf) << 12)
                        | ((str[i + 1] & 0x3f) << 6)
                        | (str[i + 2] & 0x3f);
                } else {
                    width = 4;
                    ASSERT_WIDTH(width);
                    result = ((current & 0x7) << 18)
                        | ((str[i + 1] & 0x3f) << 12)
                        | ((str[i + 2] & 0x3f) << 6)
                        | (str[i + 3] & 0x3f);
                }
                res = callback(i, width, result);
                i += width;
            }
#undef ASSERT_WIDTH
            return res;
        }

        std::string Encode(char32_t chr) const {
            char buffer[5] {0};
            Char32ToUtf8(buffer, chr);
            return std::string(buffer);
        }

        std::string Encode(std::u32string_view u32str) const override {
            std::unique_ptr<char[]> buffer(new char[u32str.size() * 4]);
            char *buffer_ptr = buffer.get();
            for (char32_t u32chr : u32str) {
                buffer_ptr += Char32ToUtf8(buffer_ptr, u32chr);
            }
            return std::string(buffer.get(), buffer_ptr - buffer.get());
        }
    };

    class Utf32LEEncoding : public Encoding {
     public:
        std::size_t CodepointCount(std::string_view view) const override {
            if (view.size() % 4 == 0) {
                return view.size() / 4;
            } else {
                throw SlokedError("UTF-32LE: Invalid length");
            }
        }

        std::pair<std::size_t, std::size_t> GetCodepoint(std::string_view view, std::size_t idx) const override {
            if (view.size() % 4 == 0) {
                return std::make_pair(idx * 4, 4);
            } else {
                throw SlokedError("UTF-32LE: Invalid length");
            }
        }

        bool IterateCodepoints(std::string_view view, std::function<bool(std::size_t, std::size_t, char32_t)> iter) const override {
            if (view.size() % 4 != 0) {
                throw SlokedError("UTF-32LE: Invalid length");
            }
            bool res = true;
            for (std::size_t i = 0; i < view.size() / 4 && res; i++) {
                char32_t chr = view[4 * i]
                    | (view[4 * i + 1] << 8)
                    | (view[4 * i + 2] << 16)
                    | (view[4 * i + 3] << 24);
                res = iter(i * 4, 4, chr);
            }
            return res;
        }

        std::string Encode(char32_t chr) const {
            char buffer[] = {
                static_cast<char>(chr & 0xff),
                static_cast<char>((chr >> 8) & 0xff),
                static_cast<char>((chr >> 16) & 0xff),
                static_cast<char>((chr >> 24) & 0xff)
            };
            return std::string(buffer, 4);
        }

        std::string Encode(std::u32string_view u32str) const {
            std::unique_ptr<char[]> buffer(new char[u32str.size() * 4]);
            char *buffer_ptr = buffer.get();
            for (std::size_t i = 0; i < u32str.size(); i++) {
                char32_t chr = u32str[i];
                *buffer_ptr++ = static_cast<char>(chr & 0xff);
                *buffer_ptr++ = static_cast<char>((chr >> 8) & 0xff);
                *buffer_ptr++ = static_cast<char>((chr >> 16) & 0xff);
                *buffer_ptr++ = static_cast<char>((chr >> 24) & 0xff);
            }
            return std::string(buffer.get(), buffer_ptr - buffer.get());
        }
    };

    static Utf8Encoding utf8Encoding;
    static Utf32LEEncoding utf32Encoding;

    const Encoding &Encoding::Utf8 = utf8Encoding;
    const Encoding &Encoding::Utf32LE = utf32Encoding;

    EncodingConverter::EncodingConverter(const Encoding &from, const Encoding &to)
        : from(from), to(to) {}

    std::string EncodingConverter::Convert(std::string_view str) const {
        return this->to.Encode(this->from.Decode(str));
    }

    std::string EncodingConverter::ReverseConvert(std::string_view str) const {
        return this->from.Encode(this->to.Decode(str));
    }

    const Encoding &EncodingConverter::GetSource() const {
        return this->from;
    }

    const Encoding &EncodingConverter::GetDestination() const {
        return this->to;
    }
}